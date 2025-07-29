#include "config.hpp"
#include "server.hpp"

#include <boost/asio/io_context.hpp>

#include <exception>
#include <iostream>

int main() {
    try {
        InitializeConfig();
        
        boost::asio::io_context io_context;
        
        auto db_service = std::make_shared<PostgresDatabase>();
        db_service->Initialize();
        
        auto session_factory = std::make_shared<SessionFactory>();

        Server s(io_context, db_service, session_factory);

        std::cout << "Server started on port " << GetConfig().GetCentralServerPort() << "...\n";
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
