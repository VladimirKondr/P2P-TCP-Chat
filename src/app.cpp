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
#include "server.hpp"

#include <boost/asio/io_context.hpp>

#include <exception>
#include <iostream>

/**
 * @brief Главная функция приложения
 *
 * Инициализирует конфигурацию, создает все необходимые компоненты
 * (базу данных, фабрику сессий, сервер) и запускает основной цикл
 * обработки событий.
 *
 * Последовательность действий:
 * 1. Инициализация конфигурации из файла/переменных окружения
 * 2. Создание контекста ввода-вывода boost::asio
 * 3. Инициализация сервиса базы данных PostgreSQL
 * 4. Создание фабрики сессий для обработки клиентов
 * 5. Запуск TCP сервера на настроенном порту
 * 6. Запуск основного цикла обработки событий
 *
 * @return int Код возврата (0 при успешном завершении)
 */
int main() {
    try {
        // Инициализируем глобальную конфигурацию из файла .config
        InitializeConfig();

        // Создаем контекст ввода-вывода для асинхронных операций
        boost::asio::io_context io_context;

        // Создаем и инициализируем сервис базы данных PostgreSQL
        auto db_service = std::make_shared<PostgresDatabase>();
        db_service->Initialize();

        // Создаем фабрику для создания HTTP сессий
        auto session_factory = std::make_shared<SessionFactory>();

        // Создаем и настраиваем TCP сервер
        const Server s(io_context, db_service, session_factory);

        std::cout << "Server started on port " << GetConfig().GetCentralServerPort() << "...\n";

        // Запускаем основной цикл обработки событий (блокирующий вызов)
        io_context.run();

    } catch (const std::exception& e) {
        // Обрабатываем любые исключения и выводим информацию об ошибке
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
