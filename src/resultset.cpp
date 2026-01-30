#include "leafodbc/resultset.h"
#include "leafodbc/common.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace leafodbc {

ResultSet::ResultSet() : current_row_(0) {
}

void ResultSet::load_from_json(const nlohmann::json& json_data) {
    rows_.clear();
    columns_.clear();
    current_row_ = 0;
    
    if (!json_data.is_array()) {
        return;
    }
    
    // Collect sample rows for schema inference
    std::vector<nlohmann::json> sample_rows;
    size_t sample_size = std::min(json_data.size(), size_t(50));
    for (size_t i = 0; i < sample_size; ++i) {
        sample_rows.push_back(json_data[i]);
    }
    
    // Load all rows
    for (const auto& row : json_data) {
        rows_.push_back(row);
    }
    
    // Infer schema
    infer_schema(sample_rows);
}

void ResultSet::add_row(const nlohmann::json& row) {
    rows_.push_back(row);
}

void ResultSet::infer_schema(const std::vector<nlohmann::json>& sample_rows) {
    if (sample_rows.empty()) {
        return;
    }
    
    // Collect all unique column names from sample rows
    std::unordered_map<std::string, bool> column_seen;
    for (const auto& row : sample_rows) {
        if (row.is_object()) {
            for (auto it = row.begin(); it != row.end(); ++it) {
                column_seen[it.key()] = true;
            }
        }
    }
    
    // Infer type for each column
    for (const auto& col_pair : column_seen) {
        const std::string& col_name = col_pair.first;
        ColumnInfo col_info;
        col_info.name = col_name;
        col_info.nullable = SQL_NULLABLE;
        col_info.sql_type = SQL_VARCHAR; // Default
        col_info.column_size = 0;
        col_info.decimal_digits = 0;
        
        // Try to infer type from sample values
        bool found_value = false;
        for (const auto& row : sample_rows) {
            if (row.is_object() && row.contains(col_name)) {
                const auto& value = row[col_name];
                if (!value.is_null()) {
                    col_info.sql_type = infer_sql_type(value);
                    found_value = true;
                    break;
                }
            }
        }
        
        col_info.type_name = infer_type_name(col_info.sql_type);
        col_info.column_size = infer_column_size(col_info.sql_type);
        
        columns_.push_back(col_info);
    }
}

SQLSMALLINT ResultSet::infer_sql_type(const nlohmann::json& value) const {
    if (value.is_null()) {
        return SQL_VARCHAR;
    } else if (value.is_boolean()) {
        return SQL_BIT;
    } else if (value.is_number_integer()) {
        int64_t v = value.get<int64_t>();
        if (v >= INT32_MIN && v <= INT32_MAX) {
            return SQL_INTEGER;
        } else {
            return SQL_BIGINT;
        }
    } else if (value.is_number_float()) {
        return SQL_DOUBLE;
    } else if (value.is_string()) {
        std::string str = value.get<std::string>();
        if (str.length() > 4000) {
            return SQL_LONGVARCHAR;
        } else {
            return SQL_VARCHAR;
        }
    } else {
        // Objects/arrays -> JSON string
        return SQL_LONGVARCHAR;
    }
}

std::string ResultSet::infer_type_name(SQLSMALLINT sql_type) const {
    switch (sql_type) {
        case SQL_BIT: return "BIT";
        case SQL_INTEGER: return "INTEGER";
        case SQL_BIGINT: return "BIGINT";
        case SQL_DOUBLE: return "DOUBLE";
        case SQL_VARCHAR: return "VARCHAR";
        case SQL_LONGVARCHAR: return "LONGVARCHAR";
        default: return "VARCHAR";
    }
}

SQLULEN ResultSet::infer_column_size(SQLSMALLINT sql_type) const {
    switch (sql_type) {
        case SQL_BIT: return 1;
        case SQL_INTEGER: return 10;
        case SQL_BIGINT: return 19;
        case SQL_DOUBLE: return 15;
        case SQL_VARCHAR: return 4000;
        case SQL_LONGVARCHAR: return 0; // Variable length
        default: return 4000;
    }
}

SQLRETURN ResultSet::fetch() {
    if (current_row_ >= rows_.size()) {
        return SQL_NO_DATA;
    }
    current_row_++;
    return SQL_SUCCESS;
}

bool ResultSet::has_column(const std::string& name) const {
    for (const auto& col : columns_) {
        if (col.name == name) {
            return true;
        }
    }
    return false;
}

SQLUSMALLINT ResultSet::get_column_index(const std::string& name) const {
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (columns_[i].name == name) {
            return static_cast<SQLUSMALLINT>(i + 1); // 1-based
        }
    }
    return 0;
}

const ColumnInfo& ResultSet::get_column_info(SQLUSMALLINT column_number) const {
    static ColumnInfo dummy;
    if (column_number < 1 || column_number > columns_.size()) {
        return dummy;
    }
    return columns_[column_number - 1];
}

SQLRETURN ResultSet::get_data(SQLUSMALLINT column_number, SQLSMALLINT target_type,
                              SQLPOINTER target_value_ptr, SQLLEN buffer_length,
                              SQLLEN* str_len_or_ind_ptr) {
    if (current_row_ == 0 || current_row_ > rows_.size()) {
        return SQL_ERROR;
    }
    
    if (column_number < 1 || column_number > columns_.size()) {
        return SQL_ERROR;
    }
    
    const ColumnInfo& col_info = columns_[column_number - 1];
    const nlohmann::json& row = rows_[current_row_ - 1];
    
    nlohmann::json value;
    if (row.is_object() && row.contains(col_info.name)) {
        value = row[col_info.name];
    } else {
        // Column not present in this row
        if (str_len_or_ind_ptr) {
            *str_len_or_ind_ptr = SQL_NULL_DATA;
        }
        return SQL_SUCCESS;
    }
    
    if (value.is_null()) {
        if (str_len_or_ind_ptr) {
            *str_len_or_ind_ptr = SQL_NULL_DATA;
        }
        return SQL_SUCCESS;
    }
    
    return convert_value(value, target_type, target_value_ptr, buffer_length, str_len_or_ind_ptr) 
           ? SQL_SUCCESS : SQL_ERROR;
}

bool ResultSet::convert_value(const nlohmann::json& value, SQLSMALLINT target_type,
                              SQLPOINTER target_value_ptr, SQLLEN buffer_length,
                              SQLLEN* str_len_or_ind_ptr) const {
    if (!target_value_ptr) {
        if (str_len_or_ind_ptr) {
            *str_len_or_ind_ptr = SQL_NULL_DATA;
        }
        return true;
    }
    
    std::string str_value;
    
    switch (target_type) {
        case SQL_C_CHAR:
        case SQL_C_WCHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR: {
            if (value.is_string()) {
                str_value = value.get<std::string>();
            } else if (value.is_number()) {
                str_value = value.dump();
            } else if (value.is_boolean()) {
                str_value = value.get<bool>() ? "1" : "0";
            } else {
                str_value = value.dump();
            }
            
            size_t copy_len = std::min(str_value.length(), static_cast<size_t>(buffer_length - 1));
            std::memcpy(target_value_ptr, str_value.c_str(), copy_len);
            static_cast<char*>(target_value_ptr)[copy_len] = '\0';
            
            if (str_len_or_ind_ptr) {
                *str_len_or_ind_ptr = static_cast<SQLLEN>(str_value.length());
            }
            return true;
        }
        
        case SQL_C_BIT: {
            bool bool_val = false;
            if (value.is_boolean()) {
                bool_val = value.get<bool>();
            } else if (value.is_number()) {
                bool_val = (value.get<int64_t>() != 0);
            } else if (value.is_string()) {
                std::string s = value.get<std::string>();
                bool_val = (s == "true" || s == "1" || s == "yes");
            }
            *static_cast<unsigned char*>(target_value_ptr) = bool_val ? 1 : 0;
            if (str_len_or_ind_ptr) {
                *str_len_or_ind_ptr = sizeof(unsigned char);
            }
            return true;
        }
        
        case SQL_C_LONG:
        case SQL_C_SLONG:
        case SQL_INTEGER: {
            int32_t int_val = 0;
            if (value.is_number_integer()) {
                int_val = static_cast<int32_t>(value.get<int64_t>());
            } else if (value.is_number_float()) {
                int_val = static_cast<int32_t>(value.get<double>());
            } else if (value.is_string()) {
                try {
                    int_val = std::stoi(value.get<std::string>());
                } catch (...) {
                    return false;
                }
            }
            *static_cast<SQLINTEGER*>(target_value_ptr) = int_val;
            if (str_len_or_ind_ptr) {
                *str_len_or_ind_ptr = sizeof(SQLINTEGER);
            }
            return true;
        }
        
        case SQL_C_SBIGINT:
        case SQL_BIGINT: {
            int64_t bigint_val = 0;
            if (value.is_number_integer()) {
                bigint_val = value.get<int64_t>();
            } else if (value.is_number_float()) {
                bigint_val = static_cast<int64_t>(value.get<double>());
            } else if (value.is_string()) {
                try {
                    bigint_val = std::stoll(value.get<std::string>());
                } catch (...) {
                    return false;
                }
            }
            *static_cast<SQLBIGINT*>(target_value_ptr) = bigint_val;
            if (str_len_or_ind_ptr) {
                *str_len_or_ind_ptr = sizeof(SQLBIGINT);
            }
            return true;
        }
        
        case SQL_C_DOUBLE:
        case SQL_DOUBLE: {
            double double_val = 0.0;
            if (value.is_number()) {
                double_val = value.get<double>();
            } else if (value.is_string()) {
                try {
                    double_val = std::stod(value.get<std::string>());
                } catch (...) {
                    return false;
                }
            }
            *static_cast<double*>(target_value_ptr) = double_val;
            if (str_len_or_ind_ptr) {
                *str_len_or_ind_ptr = sizeof(double);
            }
            return true;
        }
        
        default:
            // Fallback to string
            str_value = value.dump();
            size_t copy_len = std::min(str_value.length(), static_cast<size_t>(buffer_length - 1));
            std::memcpy(target_value_ptr, str_value.c_str(), copy_len);
            static_cast<char*>(target_value_ptr)[copy_len] = '\0';
            if (str_len_or_ind_ptr) {
                *str_len_or_ind_ptr = static_cast<SQLLEN>(str_value.length());
            }
            return true;
    }
}

void ResultSet::reset() {
    current_row_ = 0;
}

} // namespace leafodbc
