#pragma once

#include <sql.h>
#include <sqlext.h>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace leafodbc {

// Logging helper
inline bool should_log() {
    const char* env = std::getenv("LEAFODBC_LOG");
    return env && std::string(env) == "1";
}

inline void log(const std::string& message) {
    if (should_log()) {
        fprintf(stderr, "[LeafODBC] %s\n", message.c_str());
    }
}

// Error codes
constexpr SQLRETURN SQL_SUCCESS_WITH_INFO_LEAF = SQL_SUCCESS_WITH_INFO;

// Default values
constexpr const char* DEFAULT_ENDPOINT_BASE = "https://api.withleaf.io";
constexpr const char* DEFAULT_SQL_ENGINE = "SPARK_SQL";
constexpr bool DEFAULT_REMEMBER_ME = true;
constexpr int DEFAULT_TIMEOUT_SEC = 60;
constexpr bool DEFAULT_VERIFY_TLS = true;
constexpr const char* DEFAULT_USER_AGENT = "LeafODBC/0.1";

} // namespace leafodbc
