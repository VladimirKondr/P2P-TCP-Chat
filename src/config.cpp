/**
 * @file config.cpp
 * @brief Определения констант для системы конфигурации
 *
 * Этот файл содержит определения статических строковых констант,
 * которые используются как ключи для параметров конфигурации.
 *
 * Константы объявлены в config.hpp и определены здесь для:
 * - Избежания множественных определений при включении заголовка
 * - Централизованного управления именами параметров
 * - Обеспечения типобезопасности при работе с конфигурацией
 *
 * @date 2025
 */

#include "config.hpp"

// Имена параметров для центрального сервера
const char* const ConfigManager::kCentralServerHost = "CENTRAL_SERVER_HOST";
const char* const ConfigManager::kCentralServerPort = "CENTRAL_SERVER_PORT";
const char* const ConfigManager::kCentralServerAddress = "CENTRAL_SERVER_ADDRESS";

// Имена параметров для базы данных
const char* const ConfigManager::kDbHost = "DB_HOST";
const char* const ConfigManager::kDbUser = "DB_USER";
const char* const ConfigManager::kDbPassword = "DB_PASSWORD";
const char* const ConfigManager::kDbName = "DB_NAME";
const char* const ConfigManager::kDbPort = "DB_PORT";
const char* const ConfigManager::kDbConnString = "DB_CONN_STRING";

// Имена параметров для системных настроек
const char* const ConfigManager::kLogLevel = "LOG_LEVEL";
const char* const ConfigManager::kConnectionPoolSize = "CONNECTION_POOL_SIZE";
const char* const ConfigManager::kConfigFilePath = "CONFIG_FILE_PATH";
