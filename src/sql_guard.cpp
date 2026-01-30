#include "leafodbc/sql_guard.h"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace leafodbc {

std::string SQLGuard::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string SQLGuard::normalize_sql(const std::string& sql) {
    std::string normalized = trim(sql);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::toupper);
    return normalized;
}

bool SQLGuard::is_select(const std::string& sql) {
    std::string normalized = normalize_sql(sql);
    
    // Check if starts with SELECT
    if (normalized.substr(0, 6) == "SELECT") {
        return true;
    }
    
    // Check for WITH ... SELECT (CTE)
    size_t with_pos = normalized.find("WITH");
    if (with_pos != std::string::npos && with_pos < 20) {
        size_t select_pos = normalized.find("SELECT", with_pos);
        if (select_pos != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool SQLGuard::is_allowed(const std::string& sql) {
    std::string normalized = normalize_sql(sql);
    
    // Block DDL/DML keywords
    const char* blocked_keywords[] = {
        "INSERT", "UPDATE", "DELETE", "DROP", "CREATE", "ALTER",
        "TRUNCATE", "GRANT", "REVOKE", "COMMIT", "ROLLBACK"
    };
    
    for (const char* keyword : blocked_keywords) {
        size_t pos = normalized.find(keyword);
        if (pos != std::string::npos) {
            // Make sure it's a whole word (not part of another word)
            if (pos == 0 || !std::isalnum(normalized[pos - 1])) {
                size_t end = pos + strlen(keyword);
                if (end >= normalized.length() || !std::isalnum(normalized[end])) {
                    return false;
                }
            }
        }
    }
    
    // Only allow SELECT
    return is_select(sql);
}

} // namespace leafodbc
