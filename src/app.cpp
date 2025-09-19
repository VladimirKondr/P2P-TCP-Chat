/**
 * @file app.cpp
 * @brief Главная точка входа HTTP сервера приложения P2P чата
 *
 * Этот файл содержит основную функцию main() для запуска HTTP сервера,
 * который обслуживает веб-интерфейс и API для P2P чат-приложения.
 *
 * Сервер предоставляет:
 * - API для подсчета посещений
 * - Интеграцию с базой данных PostgreSQL
 *
 * @date 2025
 */

#include "config.hpp"
#include "database.hpp"
#include "server.hpp"
#include "session.hpp"

#include <boost/asio/io_context.hpp>

#include <exception>
#include <iostream>
#include <memory>

/**
 * @brief Главная функция приложения
 *
 * Инициализирует конфигурацию через dependency injection, создает все необходимые 
 * компоненты (базу данных, фабрику сессий, сервер) и запускает основной цикл
 * обработки событий.
 *
 * Последовательность действий:
 * 1. Создание менеджера конфигурации из файла .config
 * 2. Создание контекста ввода-вывода boost::asio
 * 3. Инициализация сервиса базы данных PostgreSQL с внедрением конфигурации
 * 4. Создание фабрики сессий для обработки клиентов
 * 5. Запуск TCP сервера на настроенном порту с внедрением конфигурации
 * 6. Запуск основного цикла обработки событий
 *
 * @return int Код возврата (0 при успешном завершении)
 */
int main() {
    try {
        // Создаем менеджер конфигурации и инициализируем его из файла .config
        auto config_manager = std::make_shared<ConfigManager>(".config");

        // Создаем контекст ввода-вывода для асинхронных операций
        boost::asio::io_context io_context;

        // Создаем и инициализируем сервис базы данных PostgreSQL с внедрением конфигурации
        auto db_service = std::make_shared<PostgresDatabase>(config_manager);
        db_service->Initialize();

        // Создаем фабрику для создания HTTP сессий
        auto session_factory = std::make_shared<SessionFactory>();

        // Создаем и настраиваем TCP сервер с внедрением конфигурации
        const Server s(io_context, config_manager, db_service, session_factory);

        std::cout << "Server started on port " << config_manager->GetCentralServerPort() << "...\n";

        // Запускаем основной цикл обработки событий (блокирующий вызов)
        io_context.run();

    } catch (const std::exception& e) {
        // Обрабатываем любые исключения и выводим информацию об ошибке
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
