#pragma once

#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/error_code.hpp>

#include <cstdint>
#include <memory>

using BoostTcp = boost::asio::ip::tcp;

#include "session.hpp"

class Server {
   public:
    Server(boost::asio::io_context& io_context, int16_t port)
        : acceptor_(io_context, BoostTcp::endpoint(BoostTcp::v4(), port)) {
        DoAccept();
    }

   private:
    void DoAccept() {
        acceptor_.async_accept([this](boost::system::error_code ec, BoostTcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket))->Start();
            }

            DoAccept();
        });
    }

    BoostTcp::acceptor acceptor_;
};