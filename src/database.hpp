#pragma once

#include "config.hpp"

#include <condition_variable>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <queue>
#include <string>
#include <utility>

class ConnectionPool;

/**
 * @brief RAII обертка для соединения с базой данных
 *
 * Класс Connection представляет автоматически управляемое соединение
 * с базой данных из пула соединений. При уничтожении объекта
 * соединение автоматически возвращается в пул.
 *
 * Использует паттерн RAII для безопасного управления ресурсами
 * и запрещает копирование/перемещение для обеспечения единственности.
 */
class Connection {
   public:
    /**
     * @brief Конструктор соединения
     *
     * @param conn Уникальный указатель на соединение libpqxx
     * @param pool Указатель на пул соединений для возврата соединения
     */
    Connection(std::unique_ptr<pqxx::connection> conn, ConnectionPool* pool)
        : conn_(std::move(conn)), pool_(pool) {
    }

    /**
     * @brief Деструктор - автоматически возвращает соединение в пул
     */
    ~Connection() noexcept;

    Connection(const Connection&) = delete;             ///< Запрет копирования
    Connection& operator=(const Connection&) = delete;  ///< Запрет присваивания

    Connection(Connection&&) = delete;                  ///< Запрет перемещения
    Connection& operator=(Connection&&) = delete;       ///< Запрет перемещающего присваивания

    /**
     * @brief Оператор доступа к соединению через указатель
     *
     * @return pqxx::connection* Указатель на объект соединения libpqxx
     */
    pqxx::connection* operator->() {
        return conn_.get();
    }

    /**
     * @brief Оператор разыменования для доступа к соединению
     *
     * @return pqxx::connection& Ссылка на объект соединения libpqxx
     */
    pqxx::connection& operator*() {
        return *conn_;
    }

   private:
    std::unique_ptr<pqxx::connection> conn_;  ///< Соединение с базой данных
    ConnectionPool* pool_;                    ///< Указатель на пул для возврата соединения
};

/**
 * @brief Пул соединений с базой данных
 *
 * Класс ConnectionPool управляет набором предварительно созданных
 * соединений с базой данных PostgreSQL. Обеспечивает:
 * - Эффективное переиспользование соединений
 * - Потокобезопасный доступ к соединениям
 * - Автоматическое ожидание освобождения соединений
 *
 * Использует мьютекс и условную переменную для синхронизации
 * доступа между потоками.
 */
class ConnectionPool {
   public:
    /**
     * @brief Конструктор пула соединений
     *
     * Создает указанное количество соединений с базой данных
     * и помещает их в очередь для последующего использования.
     *
     * @param size Количество соединений в пуле
     * @param options Строка подключения к PostgreSQL
     */
    ConnectionPool(uint64_t size, const std::string& options) : size_(size) {
        // Создаем все соединения заранее
        for (uint64_t i = 0; i < size_; ++i) {
            connections_.push(std::make_unique<pqxx::connection>(options));
        }
    }

    /**
     * @brief Получает соединение из пула
     *
     * Если все соединения заняты, поток будет ожидать
     * освобождения соединения другим потоком.
     *
     * @return Connection RAII-обертка для соединения
     */
    Connection Acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        // Ожидаем, пока не появится свободное соединение
        cv_.wait(lock, [this] { return !connections_.empty(); });

        // Извлекаем соединение из очереди
        auto conn = std::move(connections_.front());
        connections_.pop();
        return {std::move(conn), this};
    }

   private:
    friend class Connection;

    /**
     * @brief Возвращает соединение в пул
     *
     * Вызывается автоматически деструктором Connection.
     * Уведомляет ожидающие потоки о доступности соединения.
     *
     * @param conn Соединение для возврата в пул
     */
    void Release(std::unique_ptr<pqxx::connection> conn) {
        const std::unique_lock<std::mutex> lock(mutex_);
        connections_.push(std::move(conn));
        cv_.notify_one();                                        // Уведомляем один ожидающий поток
    }

    std::queue<std::unique_ptr<pqxx::connection>> connections_;  ///< Очередь доступных соединений
    std::mutex mutex_;                                           ///< Мьютекс для потокобезопасности
    uint64_t size_;                                              ///< Размер пула соединений
    std::condition_variable cv_;  ///< Условная переменная для ожидания
};

/**
 * @brief Реализация деструктора Connection
 *
 * Возвращает соединение в пул при уничтожении объекта.
 * Обрабатывает исключения для обеспечения безопасности в деструкторе.
 */
inline Connection::~Connection() noexcept {
    if (conn_) {
        try {
            pool_->Release(std::move(conn_));
        } catch (std::exception& e) {
            // Логируем ошибку, но не выбрасываем исключение из деструктора
            std::cout << e.what() << "\n";
        }
    }
}

/**
 * @brief Абстрактный интерфейс для работы с базой данных
 *
 * Интерфейс IDatabaseService определяет основные операции для работы
 * с базой данных в рамках приложения P2P чата:
 * - Инициализация схемы базы данных
 * - Регистрация посещений
 * - Подсчет общего количества посещений
 *
 * Применяет правило пяти с запретом копирования и перемещения.
 */
class IDatabaseService {
   public:
    virtual ~IDatabaseService() = default;

    IDatabaseService() = default;
    IDatabaseService(const IDatabaseService&) = delete;             ///< Запрет копирования
    IDatabaseService& operator=(const IDatabaseService&) = delete;  ///< Запрет присваивания
    IDatabaseService(IDatabaseService&&) = delete;                  ///< Запрет перемещения
    IDatabaseService& operator=(
        IDatabaseService&&) = delete;  ///< Запрет перемещающего присваивания

    /**
     * @brief Инициализирует схему базы данных
     *
     * Создает необходимые таблицы и структуры данных.
     * Должна быть вызвана перед началом работы с базой.
     */
    virtual void Initialize() = 0;

    /**
     * @brief Регистрирует новое посещение
     *
     * Добавляет запись о посещении с текущим временем в базу данных.
     */
    virtual void MarkVisit() = 0;

    /**
     * @brief Получает общее количество посещений
     *
     * @return uint64_t Общее число зарегистрированных посещений
     */
    virtual uint64_t GetCount() = 0;
};

/**
 * @brief Реализация сервиса базы данных для PostgreSQL
 *
 * Класс PostgresDatabase предоставляет конкретную реализацию
 * интерфейса IDatabaseService для работы с PostgreSQL.
 *
 * Особенности:
 * - Использует пул соединений для эффективной работы в многопоточной среде
 * - Автоматически создает необходимые таблицы при инициализации
 * - Поддерживает транзакции для обеспечения целостности данных
 * - Получает параметры подключения из конфигурации
 */
class PostgresDatabase : public IDatabaseService {
   public:
    /**
     * @brief Конструктор с настройкой пула соединений
     *
     * @param num_connections Количество соединений в пуле (0 = использовать значение из
     * конфигурации)
     *
     * Если num_connections равно 0, размер пула берется из конфигурации.
     * Строка подключения всегда берется из конфигурации.
     */
    explicit PostgresDatabase(uint64_t num_connections = 0)
        : conn_pool_(
              num_connections > 0 ? num_connections : GetConfig().GetConnectionPoolSize(),
              GetConfig().GetDbConnString()) {
    }

    /**
     * @brief Регистрирует посещение в базе данных
     *
     * Вставляет новую запись в таблицу visits с текущим временем.
     */
    void MarkVisit() override {
        ExecuteQuery(R"(INSERT INTO visits (time) VALUES (NOW()))");
    }

    /**
     * @brief Получает общее количество посещений
     *
     * @return uint64_t Количество записей в таблице visits
     */
    uint64_t GetCount() override {
        auto res = ExecuteQuery(R"(SELECT COUNT(*) FROM visits)");
        return res[0][0].as<uint64_t>();
    }

    /**
     * @brief Инициализирует схему базы данных
     *
     * Создает таблицу visits для хранения информации о посещениях,
     * если она еще не существует. Таблица содержит:
     * - id: автоинкрементный первичный ключ
     * - time: временная метка посещения с часовым поясом
     */
    void Initialize() override {
        ExecuteQuery(R"(CREATE TABLE IF NOT EXISTS visits (
                               id SERIAL PRIMARY KEY,
                               time TIMESTAMP WITH TIME ZONE
                               );)");
    }

   private:
    /**
     * @brief Выполняет SQL запрос с использованием транзакции
     *
     * Получает соединение из пула, создает транзакцию, выполняет запрос
     * и подтверждает транзакцию. Обеспечивает безопасное выполнение
     * операций с базой данных.
     *
     * @param query SQL запрос для выполнения
     * @return pqxx::result Результат выполнения запроса
     */
    pqxx::result ExecuteQuery(const std::string& query) {
        auto conn = conn_pool_.Acquire();                  // Получаем соединение из пула
        pqxx::work transaction(*conn);                     // Создаем транзакцию
        const pqxx::result res = transaction.exec(query);  // Выполняем запрос
        transaction.commit();                              // Подтверждаем транзакцию
        return res;
    }

    ConnectionPool conn_pool_;  ///< Пул соединений с базой данных
};