#pragma once

#include "common.h"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace leafodbc {

class LeafClient {
public:
    LeafClient(const std::string& endpoint_base, const std::string& user_agent, 
               int timeout_sec, bool verify_tls);
    
    bool authenticate(const std::string& username, const std::string& password, bool remember_me);
    bool is_authenticated() const { return !auth_token_.empty(); }
    std::string get_token() const { return auth_token_; }
    
    bool execute_query(const std::string& sql, const std::string& sql_engine, 
                      nlohmann::json& result);
    
    void set_token(const std::string& token) { auth_token_ = token; }
    void clear_token() { auth_token_.clear(); }
    
private:
    std::string endpoint_base_;
    std::string user_agent_;
    int timeout_sec_;
    bool verify_tls_;
    std::string auth_token_;
    
    std::string build_url(const std::string& path) const;
    bool http_post(const std::string& url, const std::string& body, 
                   const std::vector<std::string>& headers, std::string& response, int& status_code);
    std::string escape_json_string(const std::string& str) const;
};

} // namespace leafodbc
