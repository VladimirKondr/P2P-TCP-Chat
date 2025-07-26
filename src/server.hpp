#pragma once

#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/error_code.hpp>

#include <cstdint>
#include <memory>

using BoostTcp = boost::asio::ip::tcp;

#include "session.hpp"
#include "database.hpp"

class Server {
   public:
    Server(boost::asio::io_context& io_context, int16_t port)
        : db(Database(10)), acceptor_(io_context, BoostTcp::endpoint(BoostTcp::v4(), port)) {
        db.Initialize();
        DoAccept();
    }

   private:
    void DoAccept() {
        acceptor_.async_accept([this](boost::system::error_code ec, BoostTcp::socket socket) {
            if (!ec) {
                db.ExecuteQuery(R"(INSERT INTO visits (time) VALUES (NOW()))");
                std::make_shared<Session>(std::move(socket), &db)->Start();
            }

            DoAccept();
        });
    }

    Database db;
    BoostTcp::acceptor acceptor_;
};