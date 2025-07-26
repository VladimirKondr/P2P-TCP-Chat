#include "server.hpp"

#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <memory>

int main() {
    try {
        boost::asio::io_context io_context;

        Server s(io_context, 8000);

        std::cout << "Server started on port 8000" << "...\n";
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}