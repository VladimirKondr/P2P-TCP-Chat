#pragma once

#include "config.hpp"

#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/error_code.hpp>

#include <cstdint>
#include <memory>

using BoostTcp = boost::asio::ip::tcp;

#include "database.hpp"
#include "session.hpp"

class Server {
   public:
    Server(
        boost::asio::io_context& io_context, std::shared_ptr<IDatabaseService> db_service,
        std::shared_ptr<ISessionFactory> session_factory)
        : db_(db_service)
        , sf_(session_factory)
        , acceptor_(
              io_context, BoostTcp::endpoint(BoostTcp::v4(), GetConfig().GetCentralServerPort())) {
        db_->Initialize();
        DoAccept();
    }

   private:
    void DoAccept() {
        acceptor_.async_accept([this](boost::system::error_code ec, BoostTcp::socket socket) {
            if (!ec) {
                db_->MarkVisit();
                auto session = sf_->Create(std::move(socket), db_);
                session->Start();
            }

            DoAccept();
        });
    }

    std::shared_ptr<IDatabaseService> db_;
    std::shared_ptr<ISessionFactory> sf_;
    BoostTcp::acceptor acceptor_;
};