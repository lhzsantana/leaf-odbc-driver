#pragma once

#include "common.h"
#include <string>

namespace leafodbc {

class SQLGuard {
public:
    // Check if SQL statement is allowed (read-only: only SELECT)
    static bool is_allowed(const std::string& sql);
    
    // Check if SQL is a SELECT statement (heuristic)
    static bool is_select(const std::string& sql);
    
private:
    static std::string normalize_sql(const std::string& sql);
    static std::string trim(const std::string& str);
};

} // namespace leafodbc
