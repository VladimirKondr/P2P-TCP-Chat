#pragma once
#include <boost/program_options.hpp>

#include <functional>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

/**
 * @brief Интерфейс для управления конфигурацией приложения
 *
 * Определяет базовый контракт для получения конфигурационных параметров
 * различных типов с поддержкой значений по умолчанию.
 */
class IConfigManager {
   public:
    virtual ~IConfigManager() = default;
    
    // Запрещаем копирование и перемещение для интерфейса
    IConfigManager(const IConfigManager&) = delete;
    IConfigManager& operator=(const IConfigManager&) = delete;
    IConfigManager(IConfigManager&&) = delete;
    IConfigManager& operator=(IConfigManager&&) = delete;

   protected:
    IConfigManager() = default;

   public:
    /**
     * @brief Устанавливает значение конфигурационного параметра
     *
     * @param name Имя параметра
     * @param value Значение параметра в виде строки
     */
    virtual void Put(const std::string& name, const std::string& value) = 0;

    /**
     * @brief Проверяет наличие параметра в конфигурации
     *
     * @param name Имя параметра
     * @return true если параметр существует и не пустой, false иначе
     */
    [[nodiscard]] virtual bool Has(const std::string& name) const = 0;

    /**
     * @brief Получает строковое значение параметра
     *
     * @param name Имя параметра
     * @param default_value Значение по умолчанию, если параметр отсутствует
     * @return std::string Значение параметра или значение по умолчанию
     */
    [[nodiscard]] virtual std::string GetString(
        const std::string& name, const std::string& default_value) const = 0;
    
    /**
     * @brief Получает строковое значение параметра с пустым значением по умолчанию
     *
     * @param name Имя параметра
     * @return std::string Значение параметра или пустая строка
     */
    [[nodiscard]] std::string GetString(const std::string& name) const {
        return GetString(name, "");
    }

    /**
     * @brief Получает целочисленное значение параметра
     *
     * @param name Имя параметра
     * @param default_value Значение по умолчанию, если параметр отсутствует
     * @return int Значение параметра или значение по умолчанию
     */
    [[nodiscard]] virtual int GetInt(const std::string& name, int default_value) const = 0;
    
    /**
     * @brief Получает целочисленное значение параметра с нулевым значением по умолчанию
     *
     * @param name Имя параметра
     * @return int Значение параметра или 0
     */
    [[nodiscard]] int GetInt(const std::string& name) const {
        return GetInt(name, 0);
    }

    /**
     * @brief Получает логическое значение параметра
     *
     * @param name Имя параметра
     * @param default_value Значение по умолчанию, если параметр отсутствует
     * @return bool Значение параметра или значение по умолчанию
     */
    [[nodiscard]] virtual bool GetBool(const std::string& name, bool default_value) const = 0;
    
    /**
     * @brief Получает логическое значение параметра с false по умолчанию
     *
     * @param name Имя параметра
     * @return bool Значение параметра или false
     */
    [[nodiscard]] bool GetBool(const std::string& name) const {
        return GetBool(name, false);
    }

    // Методы для получения специфичных параметров приложения
    [[nodiscard]] virtual int GetCentralServerPort() const = 0;
    [[nodiscard]] virtual std::string GetCentralServerHost() const = 0;
    [[nodiscard]] virtual std::string GetCentralServerAddress() const = 0;
    [[nodiscard]] virtual std::string GetDbHost() const = 0;
    [[nodiscard]] virtual std::string GetDbUser() const = 0;
    [[nodiscard]] virtual std::string GetDbPassword() const = 0;
    [[nodiscard]] virtual std::string GetDbName() const = 0;
    [[nodiscard]] virtual int GetDbPort() const = 0;
    [[nodiscard]] virtual std::string GetDbConnString() const = 0;
    [[nodiscard]] virtual std::string GetLogLevel() const = 0;
    [[nodiscard]] virtual int GetConnectionPoolSize() const = 0;
    [[nodiscard]] virtual std::string GetConfigFilePath() const = 0;
};

/**
 * @brief Центральный менеджер конфигурации приложения
 *
 * Класс ConfigManager предоставляет универсальный интерфейс для управления
 * настройками приложения. Поддерживает загрузку конфигурации из файлов,
 * переменных окружения и интерполяцию значений.
 *
 * Основные возможности:
 * - Чтение конфигурации из файлов и переменных окружения
 * - Интерполяция значений с использованием синтаксиса ${variable}
 * - Типобезопасное получение значений
 * - Значения по умолчанию для отсутствующих параметров
 */
class ConfigManager : public IConfigManager {
   public:
    /**
     * @brief Конструктор по умолчанию
     */
    ConfigManager() = default;

    /**
     * @brief Инициализирует менеджер конфигурации из файла
     *
     * @param config_file Путь к файлу конфигурации
     */
    explicit ConfigManager(const std::string& config_file) {
        Initialize(config_file);
    }

    /**
     * @brief Инициализирует менеджер конфигурации
     *
     * Может быть вызван для переинициализации конфигурации с новым файлом.
     *
     * @param config_file Путь к файлу конфигурации
     */
    void Initialize(const std::string& config_file) {
        desc_.add_options()(
            "CENTRAL_SERVER_HOST", boost::program_options::value<std::string>(),
            "Host of the central server")(
            "CENTRAL_SERVER_PORT", boost::program_options::value<int>(),
            "Port of the central server")(
            "CENTRAL_SERVER_ADDRESS", boost::program_options::value<std::string>(),
            "Full address of the central server")(
            "DB_HOST", boost::program_options::value<std::string>(), "Database host")(
            "DB_USER", boost::program_options::value<std::string>(), "Database user")(
            "DB_PASSWORD", boost::program_options::value<std::string>(), "Database password")(
            "DB_NAME", boost::program_options::value<std::string>(),
            "Database name")("DB_PORT", boost::program_options::value<int>(), "Database port")(
            "DB_CONN_STRING", boost::program_options::value<std::string>(),
            "Complete database connection string")(
            "LOG_LEVEL", boost::program_options::value<std::string>(), "Logging level")(
            "CONNECTION_POOL_SIZE", boost::program_options::value<int>(),
            "Size of the database connection pool")(
            "CONFIG_FILE_PATH", boost::program_options::value<std::string>(),
            "Path to the configuration file");
        LoadConfig(config_file);
    }

    /**
     * @brief Получает значение конфигурационного параметра с приведением к указанному типу
     *
     * @tparam T Тип, к которому нужно привести значение (по умолчанию std::string)
     * @param name Имя параметра конфигурации
     * @return T Значение параметра, приведенное к типу T, или значение по умолчанию для типа T
     *
     * @note В случае ошибки преобразования возвращается значение по умолчанию для типа T
     */
    template <typename T = std::string>
    [[nodiscard]] T Get(const std::string& name) const {
        if (vm_.contains(name) && !vm_[name].empty()) {
            try {
                return vm_[name].as<T>();
            } catch (const std::exception&) {
                // В случае ошибки преобразования возвращаем значение по умолчанию
                return T{};
            }
        }
        return T{};
    }

    /**
     * @brief Устанавливает значение конфигурационного параметра
     *
     * @param name Имя параметра
     * @param value Значение параметра в виде строки
     */
    void Put(const std::string& name, const std::string& value) override {
        vm_.insert(std::make_pair(name, boost::program_options::variable_value(value, false)));
    }

    /**
     * @brief Проверяет наличие параметра в конфигурации
     *
     * @param name Имя параметра
     * @return true если параметр существует и не пустой, false иначе
     */
    [[nodiscard]] bool Has(const std::string& name) const override {
        return vm_.contains(name) && !vm_[name].empty();
    }

    /**
     * @brief Получает строковое значение параметра
     *
     * @param name Имя параметра
     * @param default_value Значение по умолчанию, если параметр отсутствует
     * @return std::string Значение параметра или значение по умолчанию
     */
    [[nodiscard]] std::string GetString(
        const std::string& name, const std::string& default_value) const override {
        return Has(name) ? Get<std::string>(name) : default_value;
    }

    /**
     * @brief Получает целочисленное значение параметра
     *
     * @param name Имя параметра
     * @param default_value Значение по умолчанию, если параметр отсутствует
     * @return int Значение параметра или значение по умолчанию
     */
    [[nodiscard]] int GetInt(const std::string& name, int default_value) const override {
        return Has(name) ? Get<int>(name) : default_value;
    }

    /**
     * @brief Получает логическое значение параметра
     *
     * @param name Имя параметра
     * @param default_value Значение по умолчанию, если параметр отсутствует
     * @return bool Значение параметра или значение по умолчанию
     */
    [[nodiscard]] bool GetBool(const std::string& name, bool default_value) const override {
        return Has(name) ? Get<bool>(name) : default_value;
    }

   private:
    boost::program_options::variables_map vm_;  ///< Карта переменных конфигурации
    boost::program_options::options_description desc_{"Configuration"};  ///< Описание опций

    /**
     * @brief Загружает конфигурацию из файла
     *
     * @param config_file Путь к файлу конфигурации
     */
    void LoadConfig(const std::string& config_file) {
        LoadConfigWithInterpolation(config_file);
    }

    /**
     * @brief Загружает конфигурацию с поддержкой интерполяции переменных
     *
     * Объединяет значения из переменных окружения и файла конфигурации,
     * разрешает вложенные ссылки на переменные в формате ${variable}.
     *
     * @param config_file Путь к файлу конфигурации
     */
    void LoadConfigWithInterpolation(const std::string& config_file) {
        boost::program_options::options_description dummy_desc;
        FillDummyDesc(dummy_desc);
        // Загружаем переменные окружения
        auto map_env = LoadEnv(dummy_desc);
        // Загружаем значения из файла
        auto map_file = LoadFile(config_file, dummy_desc);
        // Разрешаем интерполяцию переменных
        auto map_final = ResolveNested(map_env, map_file);

        // Преобразуем и сохраняем финальные значения
        for (const auto& [name, value] : map_final) {
            const boost::any any_value = ConvertValueUsingDescriptor(name, value);
            vm_.insert(
                std::make_pair(name, boost::program_options::variable_value(any_value, false)));
        }
    }

    /**
     * @brief Преобразует строковое значение к типу, определенному в дескрипторе опций
     *
     * @param name Имя параметра
     * @param value Строковое значение для преобразования
     * @return boost::any Преобразованное значение
     */
    boost::any ConvertValueUsingDescriptor(const std::string& name, const std::string& value) {
        for (const auto& option : desc_.options()) {
            if (option->long_name() == name) {
                return ConvertValueForOption(option, name, value);
            }
        }
        return value;
    }

    /**
     * @brief Преобразует значение для конкретной опции
     *
     * @param option Дескриптор опции
     * @param name Имя параметра
     * @param value Строковое значение
     * @return boost::any Преобразованное значение
     */
    static boost::any ConvertValueForOption(
        const boost::shared_ptr<boost::program_options::option_description>& option,
        const std::string& name, const std::string& value) {
        auto semantic = option->semantic();
        if (semantic) {
            return TryParseWithSemantic(semantic.get(), name, value);
        }
        return TryParseAsInt(name, value);
    }

    /**
     * @brief Пытается преобразовать значение с использованием семантики опции
     *
     * @param semantic Семантика опции boost::program_options
     * @param name Имя параметра
     * @param value Строковое значение
     * @return boost::any Преобразованное значение или обработка ошибки
     */
    static boost::any TryParseWithSemantic(
        const boost::program_options::value_semantic* semantic, const std::string& name,
        const std::string& value) {
        const std::vector<std::string> args = {value};
        try {
            boost::any result;
            semantic->parse(result, args, false);
            return result;
        } catch (const std::exception&) {
            return HandleSemanticParseFailure(name, value);
        }
    }

    /**
     * @brief Обрабатывает ошибку парсинга семантики
     *
     * @param name Имя параметра
     * @param value Строковое значение
     * @return boost::any Значение после обработки ошибки
     */
    static boost::any HandleSemanticParseFailure(
        const std::string& name, const std::string& value) {
        if (IsKnownIntOption(name)) {
            try {
                return std::stoi(value);
            } catch (const std::exception&) {
                std::cerr << "Warning: Could not convert '" << value << "' to int for " << name
                          << ", using as string\n";
            }
        }
        return value;
    }

    /**
     * @brief Пытается преобразовать значение к целому числу для известных числовых опций
     *
     * @param name Имя параметра
     * @param value Строковое значение
     * @return boost::any Целое число или исходная строка
     */
    static boost::any TryParseAsInt(const std::string& name, const std::string& value) {
        if (IsKnownIntOption(name)) {
            try {
                return std::stoi(value);
            } catch (const std::exception&) {
                return value;
            }
        }
        return value;
    }

    /**
     * @brief Проверяет, является ли опция известной числовой
     *
     * @param name Имя параметра
     * @return true если параметр должен быть числовым, false иначе
     */
    [[nodiscard]] static bool IsKnownIntOption(const std::string& name) {
        return name == "CENTRAL_SERVER_PORT" || name == "DB_PORT" || name == "CONNECTION_POOL_SIZE";
    }

    /**
     * @brief Заполняет вспомогательный дескриптор опций для парсинга
     *
     * @param dummy_desc Дескриптор для заполнения
     */
    void FillDummyDesc(boost::program_options::options_description& dummy_desc) {
        for (const auto& option : desc_.options()) {
            dummy_desc.add_options()(
                option->long_name().c_str(), boost::program_options::value<std::string>(),
                option->description().c_str());
        }
    }

    /**
     * @brief Загружает конфигурацию из переменных окружения
     *
     * @param dummy_desc Дескриптор опций для парсинга
     * @return std::unordered_map<std::string, std::string> Карта значений из окружения
     */
    static std::unordered_map<std::string, std::string> LoadEnv(
        const boost::program_options::options_description& dummy_desc) {
        std::unordered_set<std::string> valid_options;
        // Собираем все валидные имена опций
        for (const auto& option : dummy_desc.options()) {
            valid_options.insert(option->long_name());
        }

        boost::program_options::variables_map vm_env;
        std::unordered_map<std::string, std::string> map_env;

        // Парсим переменные окружения
        auto env_options = boost::program_options::parse_environment(
            dummy_desc, [&valid_options](const std::string& env_var_name) -> std::string {
                if (valid_options.contains(env_var_name)) {
                    return env_var_name;
                }
                return "";
            });

        boost::program_options::store(env_options, vm_env);
        boost::program_options::notify(vm_env);

        // Преобразуем в простую карту строк
        for (const auto& [name, value] : vm_env) {
            if (!value.empty()) {
                map_env[name] = value.as<std::string>();
            }
        }

        return map_env;
    }

    /**
     * @brief Загружает конфигурацию из файла
     *
     * @param config_file Путь к файлу конфигурации
     * @param dummy_desc Дескриптор опций для парсинга
     * @return std::unordered_map<std::string, std::string> Карта значений из файла
     */
    static std::unordered_map<std::string, std::string> LoadFile(
        const std::string& config_file,
        const boost::program_options::options_description& dummy_desc) {
        boost::program_options::variables_map vm_file;
        auto file_options =
            boost::program_options::parse_config_file(config_file.c_str(), dummy_desc);
        boost::program_options::store(file_options, vm_file);
        boost::program_options::notify(vm_file);

        std::unordered_map<std::string, std::string> map_file;
        for (const auto& [name, value] : vm_file) {
            map_file[name] = value.as<std::string>();
        }
        return map_file;
    }

    /**
     * @brief Разрешает вложенные ссылки на переменные (интерполяция)
     *
     * Объединяет значения из окружения и файла, затем разрешает ссылки
     * в формате ${variable_name}. Обнаруживает и предотвращает циклические зависимости.
     *
     * @param map_env Карта значений из переменных окружения
     * @param map_file Карта значений из файла конфигурации
     * @return std::unordered_map<std::string, std::string> Карта с разрешенными значениями
     */
    static std::unordered_map<std::string, std::string> ResolveNested(
        const std::unordered_map<std::string, std::string>& map_env,
        const std::unordered_map<std::string, std::string>& map_file) {
        std::unordered_map<std::string, std::string> map_full = map_env;
        // Значения из файла имеют приоритет над переменными окружения
        for (const auto& [name, value] : map_file) {
            map_full[name] = value;
        }

        std::unordered_map<std::string, std::string> map_resolved;
        std::unordered_set<std::string> processing;  // Для обнаружения циклических зависимостей

        // Рекурсивная функция для разрешения ссылок на переменные
        std::function<std::string(const std::string&, const std::string&)> resolve =
            [&](const std::string& key, const std::string& value) -> std::string {
            // Проверяем циклические зависимости
            if (processing.contains(key)) {
                std::cerr << "Circular dependency detected for " << key << '\n';
                return value;
            }

            // Если уже разрешено, возвращаем кешированный результат
            if (map_resolved.contains(key)) {
                return map_resolved[key];
            }

            processing.insert(key);

            std::string result = value;
            const std::regex pattern(R"(\$\{([^}]+)\})");  // Паттерн ${variable}
            std::smatch match;

            // Ищем и заменяем все вхождения ${variable}
            while (std::regex_search(result, match, pattern)) {
                const std::string placeholder = match[1].str();
                std::string replacement;

                if (map_full.contains(placeholder)) {
                    replacement = resolve(placeholder, map_full[placeholder]);
                } else {
                    // Если переменная не найдена, оставляем как есть
                    replacement = match[0].str();
                }

                result.replace(match.position(), match.length(), replacement);
            }

            processing.erase(key);
            map_resolved[key] = result;
            return result;
        };

        // Разрешаем все переменные
        for (const auto& [name, value] : map_full) {
            resolve(name, value);
        }

        return map_resolved;
    }

   public:
    // Константы для имен конфигурационных параметров
    static const char* const kCentralServerHost;     ///< Имя параметра хоста центрального сервера
    static const char* const kCentralServerPort;     ///< Имя параметра порта центрального сервера
    static const char* const kCentralServerAddress;  ///< Имя параметра полного адреса сервера
    static const char* const kDbHost;                ///< Имя параметра хоста базы данных
    static const char* const kDbUser;                ///< Имя параметра пользователя БД
    static const char* const kDbPassword;            ///< Имя параметра пароля БД
    static const char* const kDbName;                ///< Имя параметра имени БД
    static const char* const kDbPort;                ///< Имя параметра порта БД
    static const char* const kDbConnString;          ///< Имя параметра строки подключения к БД
    static const char* const kLogLevel;              ///< Имя параметра уровня логирования
    static const char* const kConnectionPoolSize;    ///< Имя параметра размера пула соединений
    static const char* const kConfigFilePath;        ///< Имя параметра пути к файлу конфигурации

    // Значения по умолчанию
    static constexpr int kDefaultCentralServerPort = 8000;  ///< Порт сервера по умолчанию
    static constexpr int kDefaultConnectionPoolSize = 10;   ///< Размер пула соединений по умолчанию
    static constexpr int kDefaultDbPort = 5432;             ///< Порт PostgreSQL по умолчанию

    /**
     * @brief Получает порт центрального сервера
     *
     * @return int Порт сервера или значение по умолчанию (8000)
     */
    [[nodiscard]] int GetCentralServerPort() const override {
        return GetInt("CENTRAL_SERVER_PORT", kDefaultCentralServerPort);
    }

    /**
     * @brief Получает хост центрального сервера
     *
     * @return std::string Хост сервера или "localhost" по умолчанию
     */
    [[nodiscard]] std::string GetCentralServerHost() const override {
        return GetString("CENTRAL_SERVER_HOST", "localhost");
    }

    /**
     * @brief Получает полный адрес центрального сервера
     *
     * Если CENTRAL_SERVER_ADDRESS не задан, формирует адрес из хоста и порта.
     *
     * @return std::string Полный адрес в формате "host:port"
     */
    [[nodiscard]] std::string GetCentralServerAddress() const override {
        std::string address = GetString("CENTRAL_SERVER_ADDRESS", "");
        if (address.empty()) {
            return GetCentralServerHost() + ":" + std::to_string(GetCentralServerPort());
        }
        return address;
    }

    /**
     * @brief Получает хост базы данных
     *
     * @return std::string Хост БД или "localhost" по умолчанию
     */
    [[nodiscard]] std::string GetDbHost() const override {
        return GetString("DB_HOST", "localhost");
    }

    /**
     * @brief Получает имя пользователя базы данных
     *
     * @return std::string Имя пользователя или "postgres" по умолчанию
     */
    [[nodiscard]] std::string GetDbUser() const override {
        return GetString("DB_USER", "postgres");
    }

    /**
     * @brief Получает пароль базы данных
     *
     * @return std::string Пароль БД (может быть пустым)
     */
    [[nodiscard]] std::string GetDbPassword() const override {
        return GetString("DB_PASSWORD", "");
    }

    /**
     * @brief Получает имя базы данных
     *
     * @return std::string Имя БД или "p2p_chat" по умолчанию
     */
    [[nodiscard]] std::string GetDbName() const override {
        return GetString("DB_NAME", "p2p_chat");
    }

    /**
     * @brief Получает порт базы данных
     *
     * @return int Порт БД или 5432 по умолчанию (стандартный порт PostgreSQL)
     */
    [[nodiscard]] int GetDbPort() const override {
        return GetInt("DB_PORT", kDefaultDbPort);
    }

    /**
     * @brief Получает строку подключения к базе данных
     *
     * Если DB_CONN_STRING не задана, формирует строку подключения PostgreSQL
     * из отдельных параметров (хост, порт, пользователь, пароль, БД).
     *
     * @return std::string Строка подключения PostgreSQL
     */
    [[nodiscard]] std::string GetDbConnString() const override {
        std::string conn_str = GetString("DB_CONN_STRING", "");
        if (conn_str.empty()) {
            return "postgresql://" + GetDbUser() + ":" + GetDbPassword() + "@" + GetDbHost() + ":" +
                   std::to_string(GetDbPort()) + "/" + GetDbName();
        }
        return conn_str;
    }

    /**
     * @brief Получает уровень логирования
     *
     * @return std::string Уровень логирования или "INFO" по умолчанию
     */
    [[nodiscard]] std::string GetLogLevel() const override {
        return GetString("LOG_LEVEL", "INFO");
    }

    /**
     * @brief Получает размер пула соединений к базе данных
     *
     * @return int Размер пула соединений или 10 по умолчанию
     */
    [[nodiscard]] int GetConnectionPoolSize() const override {
        return GetInt("CONNECTION_POOL_SIZE", kDefaultConnectionPoolSize);
    }

    /**
     * @brief Получает путь к файлу конфигурации
     *
     * @return std::string Путь к файлу конфигурации или ".config" по умолчанию
     */
    [[nodiscard]] std::string GetConfigFilePath() const override {
        return GetString("CONFIG_FILE_PATH", ".config");
    }
};
