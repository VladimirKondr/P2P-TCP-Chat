/**
 * @file main.cpp
 * @brief Простая демонстрационная программа для тестирования конфигурации
 *
 * Этот файл содержит простую демо-программу, которая:
 * - Инициализирует систему конфигурации
 * - Выводит основные параметры конфигурации
 * - Демонстрирует работу с ConfigManager
 *
 * Используется для отладки и проверки корректности настроек
 * без запуска полноценного сервера.
 *
 * @date 2025
 */

#include "config.hpp"

#include <iostream>

/**
 * @brief Демонстрационная функция для проверки конфигурации
 *
 * Создает менеджер конфигурации через dependency injection и выводит основные параметры:
 * - Порт сервера
 * - Хост базы данных
 *
 * @return int Код возврата (всегда 0)
 */
int main() {
    // Создаем менеджер конфигурации и инициализируем его из файла .config
    auto config_manager = std::make_shared<ConfigManager>(".config");

    // Выводим демонстрационную информацию
    std::cout << "Hello^ world!\n";
    std::cout << "Server will run on port: " << config_manager->GetCentralServerPort() << "\n";
    std::cout << "Database host: " << config_manager->GetDbHost() << "\n";

    return 0;
}