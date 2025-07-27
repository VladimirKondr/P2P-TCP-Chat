#pragma once

#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <pqxx/connection>
#include <pqxx/result>
#include <pqxx/transaction>
#include <queue>
#include <stdexcept>
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

class Database {
   public:
    static constexpr uint64_t kDefaultNumConnections = 10;

    // TODO(vladimirkondratyonok): [PTC-68]: fix using config class
    // NOLINTBEGIN(concurrency-mt-unsafe)
    explicit Database(uint64_t num_connections = kDefaultNumConnections)
        : conn_pool_(num_connections, [] {
            const char* conn_str = std::getenv("DB_CONN_STRING");
            if (conn_str == nullptr || *conn_str == '\0') {
                throw std::runtime_error("The environment variable DB_CONN_STRING must be set.");
            }
            return std::string(conn_str);
        }()) {
    }

    // NOLINTEND(concurrency-mt-unsafe)

    // TODO(vladimirkondratyonok): [PTC-67]: database shouldn't return pqxx::result. It should
    // return json object with data.
    pqxx::result ExecuteQuery(const std::string& query) {
        auto conn = conn_pool_.Acquire();
        pqxx::work transaction(*conn);
        const pqxx::result res = transaction.exec(query);
        transaction.commit();
        return res;
    }

    // // Execute a query without expecting results (INSERT, UPDATE, DELETE)
    // void ExecuteCommand(const std::string& command) {
    //     auto conn = conn_pool_.Acquire();
    //     pqxx::work transaction(*conn);
    //     pqxx::result res = transaction.exec(command);
    //     transaction.commit();
    // }

    void Initialize() {
        ExecuteQuery(R"(CREATE TABLE IF NOT EXISTS visits (
                               id SERIAL PRIMARY KEY,
                               time TIMESTAMP WITH TIME ZONE
                               );)");
    }

   private:
    ConnectionPool conn_pool_;
};