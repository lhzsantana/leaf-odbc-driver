#pragma once

#include "common.h"
#include <string>
#include <unordered_map>

namespace leafodbc {

struct ConnectionParams {
    std::string endpoint_base = DEFAULT_ENDPOINT_BASE;
    std::string username;
    std::string password;
    bool remember_me = DEFAULT_REMEMBER_ME;
    std::string sql_engine = DEFAULT_SQL_ENGINE;
    int timeout_sec = DEFAULT_TIMEOUT_SEC;
    bool verify_tls = DEFAULT_VERIFY_TLS;
    std::string user_agent = DEFAULT_USER_AGENT;
};

class ConnectionStringParser {
public:
    static ConnectionParams parse(const std::string& conn_str);
    static ConnectionParams parse_dsn(const std::string& dsn_name);
    static ConnectionParams merge(const ConnectionParams& dsn_params, const ConnectionParams& conn_str_params);
    
private:
    static std::string trim(const std::string& str);
    static std::string to_lower(const std::string& str);
    static bool parse_bool(const std::string& value);
    static int parse_int(const std::string& value);
    static std::unordered_map<std::string, std::string> parse_key_value_pairs(const std::string& conn_str);
};

} // namespace leafodbc
