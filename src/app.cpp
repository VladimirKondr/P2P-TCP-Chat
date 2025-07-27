#include "server.hpp"

#include <boost/asio/io_context.hpp>

#include <exception>
#include <iostream>
#include <cstdlib>

int main() {
    try {
        boost::asio::io_context io_context;

        const Server s(io_context, 8000);

        std::cout << "Server started on port 8000" << "...\n";
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}