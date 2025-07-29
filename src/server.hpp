#pragma once

#include "config.hpp"

#include <boost/asio.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/error_code.hpp>

#include <cstdint>
#include <memory>

using BoostTcp = boost::asio::ip::tcp;  ///< Псевдоним для TCP протокола boost::asio

#include "database.hpp"
#include "session.hpp"

/**
 * @brief Основной TCP сервер приложения
 *
 * Класс Server реализует асинхронный TCP сервер, который:
 * - Принимает входящие соединения на указанном порту
 * - Создает новые сессии для каждого клиента
 * - Интегрируется с базой данных для отслеживания посещений
 * - Использует паттерн Factory для создания сессий
 *
 * Сервер работает в асинхронном режиме на основе boost::asio::io_context.
 */
class Server {
   public:
    /**
     * @brief Конструктор сервера
     *
     * Создает и настраивает TCP сервер для прослушивания входящих соединений.
     * Инициализирует базу данных и начинает прием соединений.
     *
     * @param io_context Контекст ввода-вывода boost::asio для асинхронных операций
     * @param db_service Сервис базы данных для хранения информации о посещениях
     * @param session_factory Фабрика для создания новых сессий клиентов
     *
     * @note Порт для прослушивания берется из конфигурации (GetConfig().GetCentralServerPort())
     */
    Server(
        boost::asio::io_context& io_context, std::shared_ptr<IDatabaseService> db_service,
        std::shared_ptr<ISessionFactory> session_factory)
        : db_(std::move(db_service))
        , sf_(std::move(session_factory))
        , acceptor_(
              io_context, BoostTcp::endpoint(BoostTcp::v4(), GetConfig().GetCentralServerPort())) {
        db_->Initialize();  // Инициализируем базу данных (создаем таблицы если нужно)
        DoAccept();         // Начинаем принимать соединения
    }

   private:
    /**
     * @brief Асинхронно принимает новые соединения
     *
     * Метод запускает асинхронное ожидание новых TCP соединений.
     * При поступлении соединения:
     * 1. Отмечает посещение в базе данных
     * 2. Создает новую сессию через фабрику
     * 3. Запускает обработку сессии
     * 4. Рекурсивно вызывает себя для следующего соединения
     *
     * @note Метод работает в асинхронном режиме и не блокирует выполнение
     */
    void DoAccept() {
        acceptor_.async_accept([this](boost::system::error_code ec, BoostTcp::socket socket) {
            if (!ec) {
                // Регистрируем новое посещение в базе данных
                db_->MarkVisit();
                // Создаем новую сессию для клиента
                auto session = sf_->Create(std::move(socket), db_);
                // Запускаем обработку сессии
                session->Start();
            }

            // Продолжаем принимать новые соединения
            DoAccept();
        });
    }

    std::shared_ptr<IDatabaseService> db_;  ///< Сервис базы данных
    std::shared_ptr<ISessionFactory> sf_;   ///< Фабрика сессий
    BoostTcp::acceptor acceptor_;           ///< Акцептор TCP соединений
};