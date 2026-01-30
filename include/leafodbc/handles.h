#pragma once

#include "common.h"
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <cstdint>

namespace leafodbc {

// Diagnostic record
struct DiagRecord {
    std::string sqlstate;
    SQLINTEGER native_error;
    std::string message;
};

// Diagnostic stack
class DiagStack {
public:
    void add(const std::string& sqlstate, SQLINTEGER native_error, const std::string& message);
    SQLRETURN get_record(SQLSMALLINT rec_number, SQLCHAR* sqlstate, SQLINTEGER* native_error, 
                         SQLCHAR* message_text, SQLSMALLINT buffer_length, SQLSMALLINT* text_length);
    SQLRETURN get_field(SQLSMALLINT rec_number, SQLSMALLINT diag_identifier, 
                        SQLPOINTER diag_info_ptr, SQLSMALLINT buffer_length, SQLSMALLINT* string_length);
    void clear();
    SQLSMALLINT count() const { return static_cast<SQLSMALLINT>(records_.size()); }
    
private:
    std::vector<DiagRecord> records_;
    mutable std::mutex mutex_;
};

// Environment handle
struct EnvHandle {
    SQLUINTEGER odbc_version = SQL_OV_ODBC3;
    DiagStack diag;
    std::mutex mutex;
    
    bool is_valid() const { return true; }
};

// Connection handle
struct ConnHandle {
    std::string endpoint_base = DEFAULT_ENDPOINT_BASE;
    std::string username;
    std::string password;
    bool remember_me = DEFAULT_REMEMBER_ME;
    std::string sql_engine = DEFAULT_SQL_ENGINE;
    int timeout_sec = DEFAULT_TIMEOUT_SEC;
    bool verify_tls = DEFAULT_VERIFY_TLS;
    std::string user_agent = DEFAULT_USER_AGENT;
    
    // Auth state
    std::string auth_token;
    std::chrono::system_clock::time_point token_obtained_at;
    bool token_valid = false;
    
    DiagStack diag;
    std::mutex mutex;
    
    bool is_valid() const { return !endpoint_base.empty(); }
    bool is_connected() const { return token_valid && !auth_token.empty(); }
};

// Forward declaration
class ResultSet;

// Statement handle
struct StmtHandle {
    SQLHDBC conn_handle = nullptr; // Parent connection handle
    std::string sql_text;
    bool prepared = false;
    std::unique_ptr<ResultSet> resultset;
    SQLULEN current_row = 0;
    bool executed = false;
    
    DiagStack diag;
    std::mutex mutex;
    
    bool is_valid() const { return true; }
};

// Handle registry
class HandleRegistry {
public:
    static HandleRegistry& instance();
    
    SQLRETURN alloc_env(SQLHENV* env_handle);
    SQLRETURN alloc_connect(SQLHENV env_handle, SQLHDBC* conn_handle);
    SQLRETURN alloc_stmt(SQLHDBC conn_handle, SQLHSTMT* stmt_handle);
    
    SQLRETURN free_env(SQLHENV env_handle);
    SQLRETURN free_connect(SQLHDBC conn_handle);
    SQLRETURN free_stmt(SQLHSTMT stmt_handle);
    
    EnvHandle* get_env(SQLHENV env_handle);
    ConnHandle* get_conn(SQLHDBC conn_handle);
    StmtHandle* get_stmt(SQLHSTMT stmt_handle);
    
private:
    std::unordered_map<SQLHENV, std::unique_ptr<EnvHandle>> env_handles_;
    std::unordered_map<SQLHDBC, std::unique_ptr<ConnHandle>> conn_handles_;
    std::unordered_map<SQLHSTMT, std::unique_ptr<StmtHandle>> stmt_handles_;
    std::mutex mutex_;
    SQLHENV next_env_handle_ = reinterpret_cast<SQLHENV>(1);
    SQLHDBC next_conn_handle_ = reinterpret_cast<SQLHDBC>(1);
    SQLHSTMT next_stmt_handle_ = reinterpret_cast<SQLHSTMT>(1);
};

} // namespace leafodbc
