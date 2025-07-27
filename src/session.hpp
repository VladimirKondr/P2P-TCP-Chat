#pragma once

#include "database.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <cstddef>
#include <iostream>
#include <istream>
#include <memory>
#include <string>
#include <utility>

using BoostTcp = boost::asio::ip::tcp;

class ISession : public std::enable_shared_from_this<ISession> {
   public:
    virtual ~ISession() = default;
    ISession(const ISession&) = delete;
    ISession& operator=(const ISession&) = delete;
    ISession(ISession&&) = delete;
    ISession& operator=(ISession&&) = delete;
    virtual void Start() = 0;

   protected:
    ISession() = default;
};

class ISessionFactory {
   public:
    virtual ~ISessionFactory() = default;
    ISessionFactory() = default;
    ISessionFactory(const ISessionFactory&) = delete;
    ISessionFactory& operator=(const ISessionFactory&) = delete;
    ISessionFactory(ISessionFactory&&) = delete;
    ISessionFactory& operator=(ISessionFactory&&) = delete;

    virtual std::shared_ptr<ISession> Create(
        BoostTcp::socket socket, std::shared_ptr<IDatabaseService> db) = 0;
};

class Session : public ISession {
   public:
    Session(BoostTcp::socket socket, std::shared_ptr<IDatabaseService> db)
        : socket_(std::move(socket))
        , db_(std::move(db)) {  // NOLINT(hicpp-move-const-arg, performance-move-const-arg)
    }

    void Start() override {
        DoRead();
    }

   private:
    void DoRead() {
        auto self = shared_from_this();

        boost::asio::async_read_until(
            socket_, buffer_, "\r\n\r\n",
            [this, self](boost::system::error_code ec, std::size_t /*size*/) {
                if (!ec) {
                    std::istream request(&buffer_);
                    std::cout << "Received request headers:\n";
                    std::string header_line;
                    while (std::getline(request, header_line) && header_line != "\r") {
                        std::cout << header_line << '\n';
                    }
                    std::cout << "--- End of headers ---\n";

                    DoWrite();
                }
            });
    }

    void DoWrite() {
        auto self = shared_from_this();
        auto result = db_->ExecuteQuery("SELECT COUNT(*) FROM visits");
        int visit_count = 0;
        if (!result.empty()) {
            visit_count = result[0][0].as<int>();
        }

        const std::string body = "Hello, world! Visits: " + std::to_string(visit_count);
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " +
            std::to_string(body.length()) +
            "\r\n"
            "Connection: close\r\n\r\n" +
            body;
        boost::asio::async_write(
            socket_, boost::asio::buffer(response),
            [self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    std::cout << "Response sent.\n";
                }
            });
    }

    BoostTcp::socket socket_;
    boost::asio::streambuf buffer_;
    std::shared_ptr<IDatabaseService> db_;
};

class SessionFactory : public ISessionFactory {
   public:
    std::shared_ptr<ISession> Create(
        BoostTcp::socket socket, std::shared_ptr<IDatabaseService> db_service) override {
        return std::make_shared<Session>(std::move(socket), db_service);
    }
};