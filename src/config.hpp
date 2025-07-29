#pragma once
#include <boost/program_options.hpp>

#include <functional>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

class ConfigManager {
   public:
    template <typename T = std::string>
    [[nodiscard]] T Get(const std::string& name) const {
        if (vm_.contains(name) && !vm_[name].empty()) {
            try {
                return vm_[name].as<T>();
            } catch (const std::exception&) {
                return T{};
            }
        }
        return T{};
    }

    void Put(const std::string& name, const std::string& value) {
        vm_.insert(std::make_pair(name, boost::program_options::variable_value(value, false)));
    }

    [[nodiscard]] bool Has(const std::string& name) const {
        return vm_.contains(name) && !vm_[name].empty();
    }

    [[nodiscard]] std::string GetString(
        const std::string& name, const std::string& default_value = "") const {
        return Has(name) ? Get<std::string>(name) : default_value;
    }

    [[nodiscard]] int GetInt(const std::string& name, int default_value = 0) const {
        return Has(name) ? Get<int>(name) : default_value;
    }

    [[nodiscard]] bool GetBool(const std::string& name, bool default_value = false) const {
        return Has(name) ? Get<bool>(name) : default_value;
    }

   private:
    boost::program_options::variables_map vm_;
    boost::program_options::options_description desc_{"Configuration"};
    friend ConfigManager& GetConfig();
    friend void InitializeConfig(const std::string& config_file);

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

    ConfigManager() = default;

    void LoadConfig(const std::string& config_file) {
        LoadConfigWithInterpolation(config_file);
    }

    void LoadConfigWithInterpolation(const std::string& config_file) {
        boost::program_options::options_description dummy_desc;
        FillDummyDesc(dummy_desc);
        auto map_env = LoadEnv(dummy_desc);
        auto map_file = LoadFile(config_file, dummy_desc);
        auto map_final = ResolveNested(map_env, map_file);

        for (const auto& [name, value] : map_final) {
            boost::any any_value = ConvertValueUsingDescriptor(name, value);
            vm_.insert(
                std::make_pair(name, boost::program_options::variable_value(any_value, false)));
        }
    }

    boost::any ConvertValueUsingDescriptor(const std::string& name, const std::string& value) {
        for (const auto& option : desc_.options()) {
            if (option->long_name() == name) {
                return ConvertValueForOption(option, name, value);
            }
        }
        return value;
    }

    boost::any ConvertValueForOption(
        const boost::shared_ptr<boost::program_options::option_description>& option,
        const std::string& name, const std::string& value) {
        auto semantic = option->semantic();
        if (semantic) {
            return TryParseWithSemantic(semantic.get(), name, value);
        }
        return TryParseAsInt(name, value);
    }

    boost::any TryParseWithSemantic(
        const boost::program_options::value_semantic* semantic, const std::string& name,
        const std::string& value) {
        std::vector<std::string> args = {value};
        try {
            boost::any result;
            semantic->parse(result, args, false);
            return result;
        } catch (const std::exception&) {
            return HandleSemanticParseFailure(name, value);
        }
    }

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

    [[nodiscard]] static bool IsKnownIntOption(const std::string& name) {
        return name == "CENTRAL_SERVER_PORT" || name == "DB_PORT" || name == "CONNECTION_POOL_SIZE";
    }

    void FillDummyDesc(boost::program_options::options_description& dummy_desc) {
        for (const auto& option : desc_.options()) {
            dummy_desc.add_options()(
                option->long_name().c_str(), boost::program_options::value<std::string>(),
                option->description().c_str());
        }
    }

    static std::unordered_map<std::string, std::string> LoadEnv(
        const boost::program_options::options_description& dummy_desc) {
        std::unordered_set<std::string> valid_options;
        for (const auto& option : dummy_desc.options()) {
            valid_options.insert(option->long_name());
        }

        boost::program_options::variables_map vm_env;
        std::unordered_map<std::string, std::string> map_env;

        auto env_options = boost::program_options::parse_environment(
            dummy_desc, [&valid_options](const std::string& env_var_name) -> std::string {
                if (valid_options.contains(env_var_name)) {
                    return env_var_name;
                }
                return "";
            });

        boost::program_options::store(env_options, vm_env);
        boost::program_options::notify(vm_env);

        for (const auto& [name, value] : vm_env) {
            if (!value.empty()) {
                map_env[name] = value.as<std::string>();
            }
        }

        return map_env;
    }

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

    static std::unordered_map<std::string, std::string> ResolveNested(
        const std::unordered_map<std::string, std::string>& map_env,
        const std::unordered_map<std::string, std::string>& map_file) {
        std::unordered_map<std::string, std::string> map_full = map_env;
        for (const auto& [name, value] : map_file) {
            map_full[name] = value;
        }
        std::unordered_map<std::string, std::string> map_resolved;
        std::unordered_set<std::string> processing;

        std::function<std::string(const std::string&, const std::string&)> resolve =
            [&](const std::string& key, const std::string& value) -> std::string {
            if (processing.contains(key)) {
                std::cerr << "Circular dependency detected for " << key << '\n';
                return value;
            }

            if (map_resolved.contains(key)) {
                return map_resolved[key];
            }

            processing.insert(key);

            std::string result = value;
            std::regex pattern(R"(\$\{([^}]+)\})");
            std::smatch match;

            while (std::regex_search(result, match, pattern)) {
                std::string placeholder = match[1].str();
                std::string replacement;

                if (map_full.contains(placeholder)) {
                    replacement = resolve(placeholder, map_full[placeholder]);
                } else {
                    replacement = match[0].str();
                }

                result.replace(match.position(), match.length(), replacement);
            }

            processing.erase(key);
            map_resolved[key] = result;
            return result;
        };

        for (const auto& [name, value] : map_full) {
            resolve(name, value);
        }

        return map_resolved;
    }

   public:
    static const char* const kCentralServerHost;
    static const char* const kCentralServerPort;
    static const char* const kCentralServerAddress;
    static const char* const kDbHost;
    static const char* const kDbUser;
    static const char* const kDbPassword;
    static const char* const kDbName;
    static const char* const kDbPort;
    static const char* const kDbConnString;
    static const char* const kLogLevel;
    static const char* const kConnectionPoolSize;
    static const char* const kConfigFilePath;

    static constexpr int kDefaultCentralServerPort = 8000;
    static constexpr int kDefaultConnectionPoolSize = 10;
    static constexpr int kDefaultDbPort = 5432;

    [[nodiscard]] int GetCentralServerPort() const {
        return GetInt("CENTRAL_SERVER_PORT", kDefaultCentralServerPort);
    }

    [[nodiscard]] std::string GetCentralServerHost() const {
        return GetString("CENTRAL_SERVER_HOST", "localhost");
    }

    [[nodiscard]] std::string GetCentralServerAddress() const {
        std::string address = GetString("CENTRAL_SERVER_ADDRESS");
        if (address.empty()) {
            return GetCentralServerHost() + ":" + std::to_string(GetCentralServerPort());
        }
        return address;
    }

    [[nodiscard]] std::string GetDbHost() const {
        return GetString("DB_HOST", "localhost");
    }

    [[nodiscard]] std::string GetDbUser() const {
        return GetString("DB_USER", "postgres");
    }

    [[nodiscard]] std::string GetDbPassword() const {
        return GetString("DB_PASSWORD");
    }

    [[nodiscard]] std::string GetDbName() const {
        return GetString("DB_NAME", "p2p_chat");
    }

    [[nodiscard]] int GetDbPort() const {
        return GetInt("DB_PORT", kDefaultDbPort);
    }

    [[nodiscard]] std::string GetDbConnString() const {
        std::string conn_str = GetString("DB_CONN_STRING");
        if (conn_str.empty()) {
            return "postgresql://" + GetDbUser() + ":" + GetDbPassword() + "@" + GetDbHost() + ":" +
                   std::to_string(GetDbPort()) + "/" + GetDbName();
        }
        return conn_str;
    }

    [[nodiscard]] std::string GetLogLevel() const {
        return GetString("LOG_LEVEL", "INFO");
    }

    [[nodiscard]] int GetConnectionPoolSize() const {
        return GetInt("CONNECTION_POOL_SIZE", kDefaultConnectionPoolSize);
    }

    [[nodiscard]] std::string GetConfigFilePath() const {
        return GetString("CONFIG_FILE_PATH", ".config");
    }
};

inline ConfigManager& GetConfig() {
    static ConfigManager instance;
    return instance;
}

inline void InitializeConfig(const std::string& config_file = ".config") {
    auto& config = GetConfig();
    config.Initialize(config_file);
}
