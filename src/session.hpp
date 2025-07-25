#pragma once

#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <istream>
#include <memory>
#include <string>

using BoostTcp = boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
   public:
    explicit Session(BoostTcp::socket socket)
        : socket_(std::move(socket)) {  // NOLINT(hicpp-move-const-arg, performance-move-const-arg)
    }

    void Start() {
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
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 13\r\n"
            "Connection: close\r\n\r\n"
            "Hello, world!";
        boost::asio::async_write(
            socket_, boost::asio::buffer(response),
            [self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    std::cout << "Response sent.\n";
                }
            });
    }

    BoostTcp::socket socket_;
    boost::asio::streambuf buffer_{};
};