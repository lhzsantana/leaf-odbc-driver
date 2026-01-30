#include "leafodbc/handles.h"
#include "leafodbc/conn_string.h"
#include "leafodbc/leaf_client.h"
#include "leafodbc/resultset.h"
#include "leafodbc/metadata.h"
#include "leafodbc/sql_guard.h"
#include "leafodbc/common.h"
#include <sql.h>
#include <sqlext.h>
#include <cstring>
#include <memory>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

extern "C" {

// SQLAllocHandle
SQLRETURN SQLAllocHandle(SQLSMALLINT handle_type, SQLHANDLE input_handle, SQLHANDLE* output_handle) {
    if (!output_handle) {
        return SQL_ERROR;
    }
    
    auto& registry = leafodbc::HandleRegistry::instance();
    
    switch (handle_type) {
        case SQL_HANDLE_ENV:
            return registry.alloc_env(reinterpret_cast<SQLHENV*>(output_handle));
        
        case SQL_HANDLE_DBC: {
            SQLHENV env_handle = reinterpret_cast<SQLHENV>(input_handle);
            return registry.alloc_connect(env_handle, reinterpret_cast<SQLHDBC*>(output_handle));
        }
        
        case SQL_HANDLE_STMT: {
            SQLHDBC conn_handle = reinterpret_cast<SQLHDBC>(input_handle);
            return registry.alloc_stmt(conn_handle, reinterpret_cast<SQLHSTMT*>(output_handle));
        }
        
        default:
            return SQL_ERROR;
    }
}

// SQLFreeHandle
SQLRETURN SQLFreeHandle(SQLSMALLINT handle_type, SQLHANDLE handle) {
    auto& registry = leafodbc::HandleRegistry::instance();
    
    switch (handle_type) {
        case SQL_HANDLE_ENV:
            return registry.free_env(reinterpret_cast<SQLHENV>(handle));
        
        case SQL_HANDLE_DBC:
            return registry.free_connect(reinterpret_cast<SQLHDBC>(handle));
        
        case SQL_HANDLE_STMT:
            return registry.free_stmt(reinterpret_cast<SQLHSTMT>(handle));
        
        default:
            return SQL_ERROR;
    }
}

// SQLSetEnvAttr
SQLRETURN SQLSetEnvAttr(SQLHENV environment_handle, SQLINTEGER attribute, SQLPOINTER value_ptr, SQLINTEGER string_length) {
    auto* env = leafodbc::HandleRegistry::instance().get_env(environment_handle);
    if (!env) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(env->mutex);
    
    switch (attribute) {
        case SQL_ATTR_ODBC_VERSION:
            env->odbc_version = reinterpret_cast<SQLUINTEGER>(value_ptr);
            return SQL_SUCCESS;
        
        default:
            env->diag.add("HY092", 0, "Invalid attribute");
            return SQL_ERROR;
    }
}

// SQLGetEnvAttr
SQLRETURN SQLGetEnvAttr(SQLHENV environment_handle, SQLINTEGER attribute, SQLPOINTER value_ptr, SQLINTEGER buffer_length, SQLINTEGER* string_length_ptr) {
    auto* env = leafodbc::HandleRegistry::instance().get_env(environment_handle);
    if (!env) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(env->mutex);
    
    switch (attribute) {
        case SQL_ATTR_ODBC_VERSION:
            if (value_ptr) {
                *reinterpret_cast<SQLUINTEGER*>(value_ptr) = env->odbc_version;
            }
            return SQL_SUCCESS;
        
        default:
            env->diag.add("HY092", 0, "Invalid attribute");
            return SQL_ERROR;
    }
}

// SQLConnect
SQLRETURN SQLConnect(SQLHDBC connection_handle, SQLCHAR* dsn, SQLSMALLINT dsn_length,
                     SQLCHAR* uid, SQLSMALLINT uid_length,
                     SQLCHAR* pwd, SQLSMALLINT pwd_length) {
    auto* conn = leafodbc::HandleRegistry::instance().get_conn(connection_handle);
    if (!conn) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(conn->mutex);
    conn->diag.clear();
    
    std::string dsn_str;
    if (dsn) {
        if (dsn_length == SQL_NTS) {
            dsn_str = reinterpret_cast<const char*>(dsn);
        } else {
            dsn_str = std::string(reinterpret_cast<const char*>(dsn), dsn_length);
        }
    }
    
    std::string uid_str;
    if (uid) {
        if (uid_length == SQL_NTS) {
            uid_str = reinterpret_cast<const char*>(uid);
        } else {
            uid_str = std::string(reinterpret_cast<const char*>(uid), uid_length);
        }
    }
    
    std::string pwd_str;
    if (pwd) {
        if (pwd_length == SQL_NTS) {
            pwd_str = reinterpret_cast<const char*>(pwd);
        } else {
            pwd_str = std::string(reinterpret_cast<const char*>(pwd), pwd_length);
        }
    }
    
    // Parse DSN
    auto dsn_params = leafodbc::ConnectionStringParser::parse_dsn(dsn_str);
    
    // Override with UID/PWD if provided
    if (!uid_str.empty()) {
        dsn_params.username = uid_str;
    }
    if (!pwd_str.empty()) {
        dsn_params.password = pwd_str;
    }
    
    // Apply to connection handle
    conn->endpoint_base = dsn_params.endpoint_base;
    conn->username = dsn_params.username;
    conn->password = dsn_params.password;
    conn->remember_me = dsn_params.remember_me;
    conn->sql_engine = dsn_params.sql_engine;
    conn->timeout_sec = dsn_params.timeout_sec;
    conn->verify_tls = dsn_params.verify_tls;
    conn->user_agent = dsn_params.user_agent;
    
    // Authenticate
    auto client = std::make_unique<leafodbc::LeafClient>(
        conn->endpoint_base, conn->user_agent, conn->timeout_sec, conn->verify_tls);
    
    if (!client->authenticate(conn->username, conn->password, conn->remember_me)) {
        conn->diag.add("28000", 0, "Authentication failed");
        return SQL_ERROR;
    }
    
    conn->auth_token = client->get_token();
    conn->token_valid = true;
    conn->token_obtained_at = std::chrono::system_clock::now();
    
    return SQL_SUCCESS;
}

// SQLDriverConnect
SQLRETURN SQLDriverConnect(SQLHDBC connection_handle, SQLHWND window_handle,
                          SQLCHAR* in_connection_string, SQLSMALLINT string_length1,
                          SQLCHAR* out_connection_string, SQLSMALLINT buffer_length,
                          SQLSMALLINT* string_length2_ptr, SQLUSMALLINT driver_completion) {
    auto* conn = leafodbc::HandleRegistry::instance().get_conn(connection_handle);
    if (!conn) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(conn->mutex);
    conn->diag.clear();
    
    std::string conn_str;
    if (in_connection_string) {
        if (string_length1 == SQL_NTS) {
            conn_str = reinterpret_cast<const char*>(in_connection_string);
        } else {
            conn_str = std::string(reinterpret_cast<const char*>(in_connection_string), string_length1);
        }
    }
    
    // Parse connection string
    auto conn_params = leafodbc::ConnectionStringParser::parse(conn_str);
    
    // Check if DSN is specified
    std::string dsn_name;
    size_t dsn_pos = conn_str.find("DSN=");
    if (dsn_pos != std::string::npos) {
        size_t start = dsn_pos + 4;
        size_t end = conn_str.find(';', start);
        if (end == std::string::npos) end = conn_str.length();
        dsn_name = conn_str.substr(start, end - start);
    }
    
    // Merge DSN params if DSN specified
    if (!dsn_name.empty()) {
        auto dsn_params = leafodbc::ConnectionStringParser::parse_dsn(dsn_name);
        conn_params = leafodbc::ConnectionStringParser::merge(dsn_params, conn_params);
    }
    
    // Apply to connection handle
    conn->endpoint_base = conn_params.endpoint_base;
    conn->username = conn_params.username;
    conn->password = conn_params.password;
    conn->remember_me = conn_params.remember_me;
    conn->sql_engine = conn_params.sql_engine;
    conn->timeout_sec = conn_params.timeout_sec;
    conn->verify_tls = conn_params.verify_tls;
    conn->user_agent = conn_params.user_agent;
    
    // Authenticate
    auto client = std::make_unique<leafodbc::LeafClient>(
        conn->endpoint_base, conn->user_agent, conn->timeout_sec, conn->verify_tls);
    
    if (!client->authenticate(conn->username, conn->password, conn->remember_me)) {
        conn->diag.add("28000", 0, "Authentication failed");
        return SQL_ERROR;
    }
    
    conn->auth_token = client->get_token();
    conn->token_valid = true;
    conn->token_obtained_at = std::chrono::system_clock::now();
    
    // Copy connection string to output if requested
    if (out_connection_string && buffer_length > 0) {
        std::string out_str = conn_str;
        size_t copy_len = std::min(out_str.length(), static_cast<size_t>(buffer_length - 1));
        std::memcpy(out_connection_string, out_str.c_str(), copy_len);
        out_connection_string[copy_len] = '\0';
        if (string_length2_ptr) {
            *string_length2_ptr = static_cast<SQLSMALLINT>(copy_len);
        }
    }
    
    return SQL_SUCCESS;
}

// SQLDisconnect
SQLRETURN SQLDisconnect(SQLHDBC connection_handle) {
    auto* conn = leafodbc::HandleRegistry::instance().get_conn(connection_handle);
    if (!conn) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(conn->mutex);
    conn->auth_token.clear();
    conn->token_valid = false;
    
    return SQL_SUCCESS;
}

// SQLExecDirect
SQLRETURN SQLExecDirect(SQLHSTMT statement_handle, SQLCHAR* statement_text, SQLINTEGER text_length) {
    auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(statement_handle);
    if (!stmt) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(stmt->mutex);
    stmt->diag.clear();
    
    std::string sql;
    if (statement_text) {
        if (text_length == SQL_NTS) {
            sql = reinterpret_cast<const char*>(statement_text);
        } else {
            sql = std::string(reinterpret_cast<const char*>(statement_text), text_length);
        }
    }
    
    stmt->sql_text = sql;
    stmt->executed = false;
    stmt->resultset.reset();
    stmt->current_row = 0;
    
    // Check if it's a metadata query
    std::string upper_sql = sql;
    std::transform(upper_sql.begin(), upper_sql.end(), upper_sql.begin(), ::toupper);
    
    // Handle GEOMETRY_COLUMNS query
    if (upper_sql.find("GEOMETRY_COLUMNS") != std::string::npos && 
        upper_sql.find("SELECT") != std::string::npos) {
        stmt->resultset = leafodbc::Metadata::get_geometry_columns();
        stmt->executed = true;
        return SQL_SUCCESS;
    }
    
    // Check if allowed (SELECT only)
    if (!leafodbc::SQLGuard::is_allowed(sql)) {
        stmt->diag.add("42000", 0, "Only SELECT statements are allowed");
        return SQL_ERROR;
    }
    
    // Get connection handle from statement
    if (!stmt->conn_handle) {
        stmt->diag.add("08003", 0, "Connection does not exist");
        return SQL_ERROR;
    }
    
    auto* conn = leafodbc::HandleRegistry::instance().get_conn(stmt->conn_handle);
    if (!conn || !conn->is_connected()) {
        stmt->diag.add("08003", 0, "Connection not established");
        return SQL_ERROR;
    }
    
    // Execute query via LeafClient
    auto client = std::make_unique<leafodbc::LeafClient>(
        conn->endpoint_base, conn->user_agent, conn->timeout_sec, conn->verify_tls);
    client->set_token(conn->auth_token);
    
    nlohmann::json json_result;
    if (!client->execute_query(sql, conn->sql_engine, json_result)) {
        // Check if 401 - try reauth once
        if (conn->token_valid) {
            // Try reauthentication
            if (client->authenticate(conn->username, conn->password, conn->remember_me)) {
                conn->auth_token = client->get_token();
                // Retry query
                if (!client->execute_query(sql, conn->sql_engine, json_result)) {
                    stmt->diag.add("HY000", 0, "Query execution failed");
                    return SQL_ERROR;
                }
            } else {
                stmt->diag.add("28000", 0, "Reauthentication failed");
                return SQL_ERROR;
            }
        } else {
            stmt->diag.add("HY000", 0, "Query execution failed");
            return SQL_ERROR;
        }
    }
    
    // Load result set
    stmt->resultset = std::make_unique<leafodbc::ResultSet>();
    stmt->resultset->load_from_json(json_result);
    stmt->executed = true;
    stmt->current_row = 0;
    
    return SQL_SUCCESS;
}

// SQLPrepare
SQLRETURN SQLPrepare(SQLHSTMT statement_handle, SQLCHAR* statement_text, SQLINTEGER text_length) {
    auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(statement_handle);
    if (!stmt) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(stmt->mutex);
    stmt->diag.clear();
    
    std::string sql;
    if (statement_text) {
        if (text_length == SQL_NTS) {
            sql = reinterpret_cast<const char*>(statement_text);
        } else {
            sql = std::string(reinterpret_cast<const char*>(statement_text), text_length);
        }
    }
    
    stmt->sql_text = sql;
    stmt->prepared = true;
    stmt->executed = false;
    
    return SQL_SUCCESS;
}

// SQLExecute
SQLRETURN SQLExecute(SQLHSTMT statement_handle) {
    auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(statement_handle);
    if (!stmt) {
        return SQL_INVALID_HANDLE;
    }
    
    if (!stmt->prepared) {
        std::lock_guard<std::mutex> lock(stmt->mutex);
        stmt->diag.add("HY010", 0, "Function sequence error");
        return SQL_ERROR;
    }
    
    // Similar to SQLExecDirect but uses prepared statement
    return SQLExecDirect(statement_handle, 
                        reinterpret_cast<SQLCHAR*>(const_cast<char*>(stmt->sql_text.c_str())),
                        SQL_NTS);
}

// SQLFetch
SQLRETURN SQLFetch(SQLHSTMT statement_handle) {
    auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(statement_handle);
    if (!stmt) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(stmt->mutex);
    
    if (!stmt->resultset) {
        stmt->diag.add("24000", 0, "Invalid cursor state");
        return SQL_ERROR;
    }
    
    return stmt->resultset->fetch();
}

// SQLGetData
SQLRETURN SQLGetData(SQLHSTMT statement_handle, SQLUSMALLINT column_number, SQLSMALLINT target_type,
                     SQLPOINTER target_value_ptr, SQLLEN buffer_length, SQLLEN* str_len_or_ind_ptr) {
    auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(statement_handle);
    if (!stmt) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(stmt->mutex);
    
    if (!stmt->resultset) {
        stmt->diag.add("24000", 0, "Invalid cursor state");
        return SQL_ERROR;
    }
    
    return stmt->resultset->get_data(column_number, target_type, target_value_ptr, 
                                     buffer_length, str_len_or_ind_ptr);
}

// SQLGetDiagRec
SQLRETURN SQLGetDiagRec(SQLSMALLINT handle_type, SQLHANDLE handle, SQLSMALLINT rec_number,
                       SQLCHAR* sqlstate, SQLINTEGER* native_error,
                       SQLCHAR* message_text, SQLSMALLINT buffer_length, SQLSMALLINT* text_length_ptr) {
    leafodbc::DiagStack* diag = nullptr;
    
    switch (handle_type) {
        case SQL_HANDLE_ENV: {
            auto* env = leafodbc::HandleRegistry::instance().get_env(reinterpret_cast<SQLHENV>(handle));
            if (env) diag = &env->diag;
            break;
        }
        case SQL_HANDLE_DBC: {
            auto* conn = leafodbc::HandleRegistry::instance().get_conn(reinterpret_cast<SQLHDBC>(handle));
            if (conn) diag = &conn->diag;
            break;
        }
        case SQL_HANDLE_STMT: {
            auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(reinterpret_cast<SQLHSTMT>(handle));
            if (stmt) diag = &stmt->diag;
            break;
        }
        default:
            return SQL_INVALID_HANDLE;
    }
    
    if (!diag) {
        return SQL_INVALID_HANDLE;
    }
    
    return diag->get_record(rec_number, sqlstate, native_error, message_text, buffer_length, text_length_ptr);
}

// SQLGetDiagField
SQLRETURN SQLGetDiagField(SQLSMALLINT handle_type, SQLHANDLE handle, SQLSMALLINT rec_number,
                          SQLSMALLINT diag_identifier, SQLPOINTER diag_info_ptr,
                          SQLSMALLINT buffer_length, SQLSMALLINT* string_length_ptr) {
    leafodbc::DiagStack* diag = nullptr;
    
    switch (handle_type) {
        case SQL_HANDLE_ENV: {
            auto* env = leafodbc::HandleRegistry::instance().get_env(reinterpret_cast<SQLHENV>(handle));
            if (env) diag = &env->diag;
            break;
        }
        case SQL_HANDLE_DBC: {
            auto* conn = leafodbc::HandleRegistry::instance().get_conn(reinterpret_cast<SQLHDBC>(handle));
            if (conn) diag = &conn->diag;
            break;
        }
        case SQL_HANDLE_STMT: {
            auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(reinterpret_cast<SQLHSTMT>(handle));
            if (stmt) diag = &stmt->diag;
            break;
        }
        default:
            return SQL_INVALID_HANDLE;
    }
    
    if (!diag) {
        return SQL_INVALID_HANDLE;
    }
    
    return diag->get_field(rec_number, diag_identifier, diag_info_ptr, buffer_length, string_length_ptr);
}

// SQLTables
SQLRETURN SQLTables(SQLHSTMT statement_handle,
                   SQLCHAR* catalog_name, SQLSMALLINT name_length1,
                   SQLCHAR* schema_name, SQLSMALLINT name_length2,
                   SQLCHAR* table_name, SQLSMALLINT name_length3,
                   SQLCHAR* table_type, SQLSMALLINT name_length4) {
    auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(statement_handle);
    if (!stmt) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(stmt->mutex);
    stmt->diag.clear();
    
    std::string catalog_pattern = catalog_name ? 
        (name_length1 == SQL_NTS ? reinterpret_cast<const char*>(catalog_name) : 
         std::string(reinterpret_cast<const char*>(catalog_name), name_length1)) : "%";
    
    std::string schema_pattern = schema_name ?
        (name_length2 == SQL_NTS ? reinterpret_cast<const char*>(schema_name) :
         std::string(reinterpret_cast<const char*>(schema_name), name_length2)) : "%";
    
    std::string table_pattern = table_name ?
        (name_length3 == SQL_NTS ? reinterpret_cast<const char*>(table_name) :
         std::string(reinterpret_cast<const char*>(table_name), name_length3)) : "%";
    
    std::string type_pattern = table_type ?
        (name_length4 == SQL_NTS ? reinterpret_cast<const char*>(table_type) :
         std::string(reinterpret_cast<const char*>(table_type), name_length4)) : "%";
    
    stmt->resultset = leafodbc::Metadata::get_tables(catalog_pattern, schema_pattern, table_pattern, type_pattern);
    stmt->executed = true;
    stmt->current_row = 0;
    
    return SQL_SUCCESS;
}

// SQLColumns
SQLRETURN SQLColumns(SQLHSTMT statement_handle,
                    SQLCHAR* catalog_name, SQLSMALLINT name_length1,
                    SQLCHAR* schema_name, SQLSMALLINT name_length2,
                    SQLCHAR* table_name, SQLSMALLINT name_length3,
                    SQLCHAR* column_name, SQLSMALLINT name_length4) {
    auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(statement_handle);
    if (!stmt) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(stmt->mutex);
    stmt->diag.clear();
    
    std::string catalog_pattern = catalog_name ?
        (name_length1 == SQL_NTS ? reinterpret_cast<const char*>(catalog_name) :
         std::string(reinterpret_cast<const char*>(catalog_name), name_length1)) : "%";
    
    std::string schema_pattern = schema_name ?
        (name_length2 == SQL_NTS ? reinterpret_cast<const char*>(schema_name) :
         std::string(reinterpret_cast<const char*>(schema_name), name_length2)) : "%";
    
    std::string table_pattern = table_name ?
        (name_length3 == SQL_NTS ? reinterpret_cast<const char*>(table_name) :
         std::string(reinterpret_cast<const char*>(table_name), name_length3)) : "%";
    
    std::string column_pattern = column_name ?
        (name_length4 == SQL_NTS ? reinterpret_cast<const char*>(column_name) :
         std::string(reinterpret_cast<const char*>(column_name), name_length4)) : "%";
    
    stmt->resultset = leafodbc::Metadata::get_columns(catalog_pattern, schema_pattern, table_pattern, column_pattern);
    stmt->executed = true;
    stmt->current_row = 0;
    
    return SQL_SUCCESS;
}

// SQLNumResultCols
SQLRETURN SQLNumResultCols(SQLHSTMT statement_handle, SQLSMALLINT* column_count_ptr) {
    auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(statement_handle);
    if (!stmt) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(stmt->mutex);
    
    if (!stmt->resultset) {
        if (column_count_ptr) {
            *column_count_ptr = 0;
        }
        return SQL_SUCCESS;
    }
    
    if (column_count_ptr) {
        *column_count_ptr = stmt->resultset->get_column_count();
    }
    
    return SQL_SUCCESS;
}

// SQLDescribeCol
SQLRETURN SQLDescribeCol(SQLHSTMT statement_handle, SQLUSMALLINT column_number,
                        SQLCHAR* column_name, SQLSMALLINT buffer_length,
                        SQLSMALLINT* name_length_ptr, SQLSMALLINT* data_type_ptr,
                        SQLULEN* column_size_ptr, SQLSMALLINT* decimal_digits_ptr,
                        SQLSMALLINT* nullable_ptr) {
    auto* stmt = leafodbc::HandleRegistry::instance().get_stmt(statement_handle);
    if (!stmt) {
        return SQL_INVALID_HANDLE;
    }
    
    std::lock_guard<std::mutex> lock(stmt->mutex);
    
    if (!stmt->resultset) {
        stmt->diag.add("24000", 0, "Invalid cursor state");
        return SQL_ERROR;
    }
    
    const auto& col_info = stmt->resultset->get_column_info(column_number);
    
    if (column_name && buffer_length > 0) {
        size_t copy_len = std::min(col_info.name.length(), static_cast<size_t>(buffer_length - 1));
        std::memcpy(column_name, col_info.name.c_str(), copy_len);
        column_name[copy_len] = '\0';
        if (name_length_ptr) {
            *name_length_ptr = static_cast<SQLSMALLINT>(copy_len);
        }
    } else if (name_length_ptr) {
        *name_length_ptr = static_cast<SQLSMALLINT>(col_info.name.length());
    }
    
    if (data_type_ptr) {
        *data_type_ptr = col_info.sql_type;
    }
    if (column_size_ptr) {
        *column_size_ptr = col_info.column_size;
    }
    if (decimal_digits_ptr) {
        *decimal_digits_ptr = col_info.decimal_digits;
    }
    if (nullable_ptr) {
        *nullable_ptr = col_info.nullable;
    }
    
    return SQL_SUCCESS;
}

} // extern "C"
