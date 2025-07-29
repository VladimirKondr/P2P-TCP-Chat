#pragma once

#include "database.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <cstddef>
#include <iostream>
#include <istream>
#include <memory>
#include <string>
#include <utility>

using BoostTcp = boost::asio::ip::tcp;  ///< Псевдоним для TCP протокола boost::asio

/**
 * @brief Абстрактный интерфейс для сессии клиента
 *
 * Базовый класс для всех типов сессий. Использует паттерн CRTP
 * (enable_shared_from_this) для безопасного получения shared_ptr на себя
 * в асинхронных операциях.
 *
 * Реализует правило пяти (Rule of Five) с запретом копирования и перемещения
 * для обеспечения единственности экземпляра сессии.
 */
class ISession : public std::enable_shared_from_this<ISession> {
   public:
    virtual ~ISession() = default;
    ISession(const ISession&) = delete;             ///< Запрет копирования
    ISession& operator=(const ISession&) = delete;  ///< Запрет присваивания
    ISession(ISession&&) = delete;                  ///< Запрет перемещения
    ISession& operator=(ISession&&) = delete;       ///< Запрет перемещающего присваивания

    /**
     * @brief Запускает обработку сессии
     *
     * Абстрактный метод, который должен быть реализован в наследниках
     * для начала обработки клиентского соединения.
     */
    virtual void Start() = 0;

   protected:
    /**
     * @brief Защищенный конструктор по умолчанию
     *
     * Конструктор сделан защищенным, чтобы предотвратить прямое
     * создание экземпляров абстрактного класса.
     */
    ISession() = default;
};

/**
 * @brief Абстрактная фабрика для создания сессий
 *
 * Интерфейс фабрики, реализующий паттерн Abstract Factory для создания
 * различных типов сессий. Также применяет правило пяти с запретом
 * копирования и перемещения.
 */
class ISessionFactory {
   public:
    virtual ~ISessionFactory() = default;
    ISessionFactory() = default;
    ISessionFactory(const ISessionFactory&) = delete;             ///< Запрет копирования
    ISessionFactory& operator=(const ISessionFactory&) = delete;  ///< Запрет присваивания
    ISessionFactory(ISessionFactory&&) = delete;                  ///< Запрет перемещения
    ISessionFactory& operator=(ISessionFactory&&) = delete;  ///< Запрет перемещающего присваивания

    /**
     * @brief Создает новую сессию для клиента
     *
     * @param socket TCP сокет клиентского соединения
     * @param db Сервис базы данных для работы с данными
     * @return std::shared_ptr<ISession> Умный указатель на созданную сессию
     */
    virtual std::shared_ptr<ISession> Create(
        BoostTcp::socket socket, std::shared_ptr<IDatabaseService> db) = 0;
};

/**
 * @brief Конкретная реализация сессии HTTP
 *
 * Класс Session реализует простой HTTP сервер, который:
 * - Читает HTTP заголовки от клиента
 * - Отвечает статическим HTML с информацией о количестве посещений
 * - Закрывает соединение после отправки ответа
 *
 * Сессия работает в асинхронном режиме с использованием boost::asio.
 */
class Session : public ISession {
   public:
    /**
     * @brief Конструктор сессии
     *
     * @param socket TCP сокет для коммуникации с клиентом
     * @param db Сервис базы данных для получения информации о посещениях
     */
    Session(BoostTcp::socket socket, std::shared_ptr<IDatabaseService> db)
        : socket_(std::move(socket))
        , db_(std::move(db)) {  // NOLINT(hicpp-move-const-arg, performance-move-const-arg)
    }

    /**
     * @brief Запускает обработку HTTP сессии
     *
     * Начинает асинхронное чтение HTTP заголовков от клиента.
     */
    void Start() override {
        DoRead();
    }

   private:
    /**
     * @brief Асинхронно читает HTTP заголовки от клиента
     *
     * Читает данные до последовательности "\r\n\r\n", которая обозначает
     * конец HTTP заголовков. После получения заголовков выводит их в консоль
     * и переходит к отправке ответа.
     */
    void DoRead() {
        auto self = shared_from_this();  // Сохраняем сессию в памяти во время асинхронной операции

        boost::asio::async_read_until(
            socket_, buffer_, "\r\n\r\n",
            [this, self](boost::system::error_code ec, std::size_t /*size*/) {
                if (!ec) {
                    std::istream request(&buffer_);
                    std::cout << "Received request headers:\n";

                    // Читаем и выводим все строки заголовков
                    std::string header_line;
                    while (std::getline(request, header_line) && header_line != "\r") {
                        std::cout << header_line << '\n';
                    }
                    std::cout << "--- End of headers ---\n";

                    DoWrite();  // Переходим к отправке ответа
                }
            });
    }

    /**
     * @brief Асинхронно отправляет HTTP ответ клиенту
     *
     * Формирует HTTP ответ с информацией о количестве посещений
     * и отправляет его клиенту. После отправки соединение закрывается.
     */
    void DoWrite() {
        auto self = shared_from_this();  // Сохраняем сессию в памяти во время асинхронной операции

        // Получаем количество посещений из базы данных
        const uint64_t visit_count = db_->GetCount();

        // Формируем тело HTTP ответа
        const std::string body = "Hello, world! Visits: " + std::to_string(visit_count);

        // Формируем полный HTTP ответ с заголовками
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " +
            std::to_string(body.length()) +
            "\r\n"
            "Connection: close\r\n\r\n" +
            body;

        // Асинхронно отправляем ответ клиенту
        boost::asio::async_write(
            socket_, boost::asio::buffer(response),
            [self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    std::cout << "Response sent.\n";
                }
                // Соединение автоматически закроется при уничтожении сессии
            });
    }

    BoostTcp::socket socket_;               ///< TCP сокет для коммуникации с клиентом
    boost::asio::streambuf buffer_;         ///< Буфер для чтения HTTP данных
    std::shared_ptr<IDatabaseService> db_;  ///< Сервис базы данных
};

/**
 * @brief Конкретная реализация фабрики сессий
 *
 * Фабрика, которая создает экземпляры Session для обработки HTTP запросов.
 * Реализует паттерн Factory Method.
 */
class SessionFactory : public ISessionFactory {
   public:
    /**
     * @brief Создает новую HTTP сессию
     *
     * @param socket TCP сокет клиентского соединения
     * @param db_service Сервис базы данных для работы с посещениями
     * @return std::shared_ptr<ISession> Умный указатель на созданную сессию
     */
    std::shared_ptr<ISession> Create(
        BoostTcp::socket socket, std::shared_ptr<IDatabaseService> db_service) override {
        return std::make_shared<Session>(std::move(socket), db_service);
    }
};