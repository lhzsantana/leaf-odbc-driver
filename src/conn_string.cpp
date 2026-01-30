#include "leafodbc/conn_string.h"
#include "leafodbc/common.h"
#include <cctype>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <vector>

namespace leafodbc {

std::string ConnectionStringParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string ConnectionStringParser::to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool ConnectionStringParser::parse_bool(const std::string& value) {
    std::string lower = to_lower(trim(value));
    return lower == "true" || lower == "1" || lower == "yes" || lower == "on";
}

int ConnectionStringParser::parse_int(const std::string& value) {
    try {
        return std::stoi(trim(value));
    } catch (...) {
        return 0;
    }
}

std::unordered_map<std::string, std::string> ConnectionStringParser::parse_key_value_pairs(const std::string& conn_str) {
    std::unordered_map<std::string, std::string> params;
    std::string current_key;
    std::string current_value;
    bool in_key = true;
    bool in_quoted_value = false;
    
    for (size_t i = 0; i < conn_str.length(); ++i) {
        char c = conn_str[i];
        
        if (in_quoted_value) {
            if (c == '"' && (i == 0 || conn_str[i-1] != '\\')) {
                in_quoted_value = false;
                in_key = true;
                params[to_lower(current_key)] = current_value;
                current_key.clear();
                current_value.clear();
            } else {
                current_value += c;
            }
        } else if (c == '=' && in_key && !current_key.empty()) {
            in_key = false;
        } else if (c == ';' || c == '\0') {
            if (!current_key.empty()) {
                params[to_lower(current_key)] = trim(current_value);
                current_key.clear();
                current_value.clear();
            }
            in_key = true;
        } else if (c == '"' && !in_key) {
            in_quoted_value = true;
        } else if (in_key) {
            current_key += c;
        } else {
            current_value += c;
        }
    }
    
    if (!current_key.empty()) {
        params[to_lower(current_key)] = trim(current_value);
    }
    
    return params;
}

ConnectionParams ConnectionStringParser::parse(const std::string& conn_str) {
    ConnectionParams params;
    auto pairs = parse_key_value_pairs(conn_str);
    
    for (const auto& pair : pairs) {
        const std::string& key = pair.first;
        const std::string& value = pair.second;
        
        if (key == "endpointbase" || key == "endpoint_base") {
            params.endpoint_base = value;
        } else if (key == "username" || key == "uid" || key == "user") {
            params.username = value;
        } else if (key == "password" || key == "pwd") {
            params.password = value;
        } else if (key == "rememberme" || key == "remember_me") {
            params.remember_me = parse_bool(value);
        } else if (key == "sqlengine" || key == "sql_engine") {
            params.sql_engine = value;
        } else if (key == "timeoutsec" || key == "timeout_sec" || key == "timeout") {
            params.timeout_sec = parse_int(value);
            if (params.timeout_sec <= 0) params.timeout_sec = DEFAULT_TIMEOUT_SEC;
        } else if (key == "verifytls" || key == "verify_tls" || key == "sslverify") {
            params.verify_tls = parse_bool(value);
        } else if (key == "useragent" || key == "user_agent") {
            params.user_agent = value;
        }
    }
    
    return params;
}

ConnectionParams ConnectionStringParser::parse_dsn(const std::string& dsn_name) {
    ConnectionParams params;
    
    // Try to read from odbc.ini
    // Common locations: ~/.odbc.ini, /etc/odbc.ini, /usr/local/etc/odbc.ini
    std::vector<std::string> odbc_ini_paths = {
        std::string(std::getenv("HOME") ? std::getenv("HOME") : "") + "/.odbc.ini",
        "/etc/odbc.ini",
        "/usr/local/etc/odbc.ini"
    };
    
    std::ifstream file;
    for (const auto& path : odbc_ini_paths) {
        file.open(path);
        if (file.is_open()) {
            log("Reading DSN from " + path);
            break;
        }
    }
    
    if (!file.is_open()) {
        log("Could not find odbc.ini file");
        return params;
    }
    
    std::string line;
    bool in_section = false;
    std::string current_section;
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        if (line[0] == '[' && line.back() == ']') {
            current_section = trim(line.substr(1, line.length() - 2));
            in_section = (current_section == dsn_name);
            continue;
        }
        
        if (in_section) {
            size_t eq_pos = line.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = to_lower(trim(line.substr(0, eq_pos)));
                std::string value = trim(line.substr(eq_pos + 1));
                
                if (key == "endpointbase" || key == "endpoint_base") {
                    params.endpoint_base = value;
                } else if (key == "username" || key == "uid" || key == "user") {
                    params.username = value;
                } else if (key == "password" || key == "pwd") {
                    params.password = value;
                } else if (key == "rememberme" || key == "remember_me") {
                    params.remember_me = parse_bool(value);
                } else if (key == "sqlengine" || key == "sql_engine") {
                    params.sql_engine = value;
                } else if (key == "timeoutsec" || key == "timeout_sec" || key == "timeout") {
                    params.timeout_sec = parse_int(value);
                    if (params.timeout_sec <= 0) params.timeout_sec = DEFAULT_TIMEOUT_SEC;
                } else if (key == "verifytls" || key == "verify_tls" || key == "sslverify") {
                    params.verify_tls = parse_bool(value);
                } else if (key == "useragent" || key == "user_agent") {
                    params.user_agent = value;
                }
            }
        }
    }
    
    return params;
}

ConnectionParams ConnectionStringParser::merge(const ConnectionParams& dsn_params, const ConnectionParams& conn_str_params) {
    ConnectionParams merged = dsn_params;
    
    // Connection string overrides DSN
    if (!conn_str_params.endpoint_base.empty()) {
        merged.endpoint_base = conn_str_params.endpoint_base;
    }
    if (!conn_str_params.username.empty()) {
        merged.username = conn_str_params.username;
    }
    if (!conn_str_params.password.empty()) {
        merged.password = conn_str_params.password;
    }
    if (conn_str_params.remember_me != DEFAULT_REMEMBER_ME) {
        merged.remember_me = conn_str_params.remember_me;
    }
    if (!conn_str_params.sql_engine.empty()) {
        merged.sql_engine = conn_str_params.sql_engine;
    }
    if (conn_str_params.timeout_sec > 0) {
        merged.timeout_sec = conn_str_params.timeout_sec;
    }
    if (conn_str_params.verify_tls != DEFAULT_VERIFY_TLS) {
        merged.verify_tls = conn_str_params.verify_tls;
    }
    if (!conn_str_params.user_agent.empty()) {
        merged.user_agent = conn_str_params.user_agent;
    }
    
    return merged;
}

} // namespace leafodbc
