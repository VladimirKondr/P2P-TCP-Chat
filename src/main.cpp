#include "config.hpp"

#include <iostream>

int main() {
    InitializeConfig();
    auto& config = GetConfig();
    
    std::cout << "Hello^ world!\n";
    std::cout << "Server will run on port: " << config.GetCentralServerPort() << "\n";
    std::cout << "Database host: " << config.GetDbHost() << "\n";
    
    return 0;
}