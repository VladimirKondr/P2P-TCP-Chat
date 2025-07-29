#pragma once

#include "config.hpp"

#include <condition_variable>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <queue>
#include <string>
#include <utility>

class ConnectionPool;

class Connection {
   public:
    Connection(std::unique_ptr<pqxx::connection> conn, ConnectionPool* pool)
        : conn_(std::move(conn)), pool_(pool) {
    }

    ~Connection() noexcept;

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&&) = delete;
    Connection& operator=(Connection&&) = delete;

    pqxx::connection* operator->() {
        return conn_.get();
    }

    pqxx::connection& operator*() {
        return *conn_;
    }

   private:
    std::unique_ptr<pqxx::connection> conn_;
    ConnectionPool* pool_;
};

class ConnectionPool {
   public:
    ConnectionPool(uint64_t size, const std::string& options) : size_(size) {
        for (uint64_t i = 0; i < size_; ++i) {
            connections_.push(std::make_unique<pqxx::connection>(options));
        }
    }

    Connection Acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !connections_.empty(); });

        auto conn = std::move(connections_.front());
        connections_.pop();
        return {std::move(conn), this};
    }

   private:
    friend class Connection;

    void Release(std::unique_ptr<pqxx::connection> conn) {
        const std::unique_lock<std::mutex> lock(mutex_);
        connections_.push(std::move(conn));
        cv_.notify_one();
    }

    std::queue<std::unique_ptr<pqxx::connection>> connections_;
    std::mutex mutex_;
    uint64_t size_;
    std::condition_variable cv_;
};

inline Connection::~Connection() noexcept {
    if (conn_) {
        try {
            pool_->Release(std::move(conn_));
        } catch (std::exception& e) {
            std::cout << e.what() << "\n";
        }
    }
}

class IDatabaseService {
   public:
    virtual ~IDatabaseService() = default;

    IDatabaseService() = default;
    IDatabaseService(const IDatabaseService&) = delete;
    IDatabaseService& operator=(const IDatabaseService&) = delete;
    IDatabaseService(IDatabaseService&&) = delete;
    IDatabaseService& operator=(IDatabaseService&&) = delete;

    virtual void Initialize() = 0;

    virtual void MarkVisit() = 0;

    virtual uint64_t GetCount() = 0;
};

class PostgresDatabase : public IDatabaseService {
   public:
    explicit PostgresDatabase(uint64_t num_connections = 0)
        : conn_pool_(
              num_connections > 0 ? num_connections : GetConfig().GetConnectionPoolSize(),
              GetConfig().GetDbConnString()) {
    }

    void MarkVisit() override {
        ExecuteQuery(R"(INSERT INTO visits (time) VALUES (NOW()))");
    }

    uint64_t GetCount() override {
        auto res = ExecuteQuery(R"(SELECT COUNT(*) FROM visits)");
        if (res.empty() || res[0].size() == 0) {
            return 0;
        }
        return res[0][0].as<uint64_t>();
    }

    void Initialize() override {
        ExecuteQuery(R"(CREATE TABLE IF NOT EXISTS visits (
                               id SERIAL PRIMARY KEY,
                               time TIMESTAMP WITH TIME ZONE
                               );)");
    }

   private:
    pqxx::result ExecuteQuery(const std::string& query) {
        auto conn = conn_pool_.Acquire();
        pqxx::work transaction(*conn);
        const pqxx::result res = transaction.exec(query);
        transaction.commit();
        return res;
    }

    ConnectionPool conn_pool_;
};