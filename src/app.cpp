#include "server.hpp"

#include <boost/asio/io_context.hpp>

#include <exception>
#include <iostream>

int main() {
    try {
        boost::asio::io_context io_context;
        auto db_service = std::make_shared<PostgresDatabase>(10);
        db_service->Initialize();
        
        auto session_factory = std::make_shared<SessionFactory>();

        const Server s(io_context, 8000, db_service, session_factory);

        std::cout << "Server started on port 8000" << "...\n";
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}