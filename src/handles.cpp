#include "leafodbc/handles.h"
#include "leafodbc/common.h"
#include "leafodbc/resultset.h"
#include <algorithm>
#include <cstring>

namespace leafodbc {

void DiagStack::add(const std::string& sqlstate, SQLINTEGER native_error, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    DiagRecord rec;
    rec.sqlstate = sqlstate;
    rec.native_error = native_error;
    rec.message = message;
    records_.insert(records_.begin(), rec); // Most recent first
    if (records_.size() > 10) {
        records_.resize(10); // Keep only last 10
    }
}

SQLRETURN DiagStack::get_record(SQLSMALLINT rec_number, SQLCHAR* sqlstate, SQLINTEGER* native_error,
                                SQLCHAR* message_text, SQLSMALLINT buffer_length, SQLSMALLINT* text_length) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (rec_number < 1 || rec_number > static_cast<SQLSMALLINT>(records_.size())) {
        return SQL_NO_DATA;
    }
    
    const DiagRecord& rec = records_[rec_number - 1];
    
    if (sqlstate) {
        size_t len = std::min(rec.sqlstate.size(), static_cast<size_t>(5));
        std::memcpy(sqlstate, rec.sqlstate.c_str(), len);
        if (len < 5) sqlstate[len] = '\0';
    }
    
    if (native_error) {
        *native_error = rec.native_error;
    }
    
    if (message_text && buffer_length > 0) {
        size_t len = std::min(rec.message.size(), static_cast<size_t>(buffer_length - 1));
        std::memcpy(message_text, rec.message.c_str(), len);
        message_text[len] = '\0';
        if (text_length) {
            *text_length = static_cast<SQLSMALLINT>(len);
        }
    } else if (text_length) {
        *text_length = static_cast<SQLSMALLINT>(rec.message.size());
    }
    
    return SQL_SUCCESS;
}

SQLRETURN DiagStack::get_field(SQLSMALLINT rec_number, SQLSMALLINT diag_identifier,
                               SQLPOINTER diag_info_ptr, SQLSMALLINT buffer_length, SQLSMALLINT* string_length) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (rec_number < 1 || rec_number > static_cast<SQLSMALLINT>(records_.size())) {
        return SQL_NO_DATA;
    }
    
    const DiagRecord& rec = records_[rec_number - 1];
    
    switch (diag_identifier) {
        case SQL_DIAG_SQLSTATE:
            if (diag_info_ptr && buffer_length >= 6) {
                size_t len = std::min(rec.sqlstate.size(), static_cast<size_t>(5));
                std::memcpy(diag_info_ptr, rec.sqlstate.c_str(), len);
                static_cast<SQLCHAR*>(diag_info_ptr)[len] = '\0';
                if (string_length) {
                    *string_length = static_cast<SQLSMALLINT>(len);
                }
                return SQL_SUCCESS;
            }
            break;
        case SQL_DIAG_NATIVE:
            if (diag_info_ptr) {
                *static_cast<SQLINTEGER*>(diag_info_ptr) = rec.native_error;
                return SQL_SUCCESS;
            }
            break;
        case SQL_DIAG_MESSAGE_TEXT:
            if (diag_info_ptr && buffer_length > 0) {
                size_t len = std::min(rec.message.size(), static_cast<size_t>(buffer_length - 1));
                std::memcpy(diag_info_ptr, rec.message.c_str(), len);
                static_cast<SQLCHAR*>(diag_info_ptr)[len] = '\0';
                if (string_length) {
                    *string_length = static_cast<SQLSMALLINT>(len);
                }
                return SQL_SUCCESS;
            }
            break;
    }
    
    return SQL_ERROR;
}

void DiagStack::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
}

HandleRegistry& HandleRegistry::instance() {
    static HandleRegistry reg;
    return reg;
}

SQLRETURN HandleRegistry::alloc_env(SQLHENV* env_handle) {
    if (!env_handle) {
        return SQL_ERROR;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto handle = std::make_unique<EnvHandle>();
    SQLHENV h = reinterpret_cast<SQLHENV>(reinterpret_cast<uintptr_t>(next_env_handle_) + 1);
    next_env_handle_ = h;
    env_handles_[h] = std::move(handle);
    *env_handle = h;
    return SQL_SUCCESS;
}

SQLRETURN HandleRegistry::alloc_connect(SQLHENV env_handle, SQLHDBC* conn_handle) {
    if (!conn_handle) {
        return SQL_ERROR;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (env_handles_.find(env_handle) == env_handles_.end()) {
        return SQL_INVALID_HANDLE;
    }
    
    auto handle = std::make_unique<ConnHandle>();
    SQLHDBC h = reinterpret_cast<SQLHDBC>(reinterpret_cast<uintptr_t>(next_conn_handle_) + 1);
    next_conn_handle_ = h;
    conn_handles_[h] = std::move(handle);
    *conn_handle = h;
    return SQL_SUCCESS;
}

SQLRETURN HandleRegistry::alloc_stmt(SQLHDBC conn_handle, SQLHSTMT* stmt_handle) {
    if (!stmt_handle) {
        return SQL_ERROR;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (conn_handles_.find(conn_handle) == conn_handles_.end()) {
        return SQL_INVALID_HANDLE;
    }
    
    auto handle = std::make_unique<StmtHandle>();
    handle->conn_handle = conn_handle; // Store parent connection
    SQLHSTMT h = reinterpret_cast<SQLHSTMT>(reinterpret_cast<uintptr_t>(next_stmt_handle_) + 1);
    next_stmt_handle_ = h;
    stmt_handles_[h] = std::move(handle);
    *stmt_handle = h;
    return SQL_SUCCESS;
}

SQLRETURN HandleRegistry::free_env(SQLHENV env_handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = env_handles_.find(env_handle);
    if (it == env_handles_.end()) {
        return SQL_INVALID_HANDLE;
    }
    env_handles_.erase(it);
    return SQL_SUCCESS;
}

SQLRETURN HandleRegistry::free_connect(SQLHDBC conn_handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = conn_handles_.find(conn_handle);
    if (it == conn_handles_.end()) {
        return SQL_INVALID_HANDLE;
    }
    conn_handles_.erase(it);
    return SQL_SUCCESS;
}

SQLRETURN HandleRegistry::free_stmt(SQLHSTMT stmt_handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = stmt_handles_.find(stmt_handle);
    if (it == stmt_handles_.end()) {
        return SQL_INVALID_HANDLE;
    }
    stmt_handles_.erase(it);
    return SQL_SUCCESS;
}

EnvHandle* HandleRegistry::get_env(SQLHENV env_handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = env_handles_.find(env_handle);
    return (it != env_handles_.end()) ? it->second.get() : nullptr;
}

ConnHandle* HandleRegistry::get_conn(SQLHDBC conn_handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = conn_handles_.find(conn_handle);
    return (it != conn_handles_.end()) ? it->second.get() : nullptr;
}

StmtHandle* HandleRegistry::get_stmt(SQLHSTMT stmt_handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = stmt_handles_.find(stmt_handle);
    return (it != stmt_handles_.end()) ? it->second.get() : nullptr;
}

} // namespace leafodbc
