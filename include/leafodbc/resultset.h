#pragma once

#include "common.h"
#include <sql.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace leafodbc {

struct ColumnInfo {
    std::string name;
    SQLSMALLINT sql_type;
    SQLULEN column_size;
    SQLSMALLINT decimal_digits;
    SQLSMALLINT nullable;
    std::string type_name;
};

class ResultSet {
public:
    ResultSet();
    
    void load_from_json(const nlohmann::json& json_data);
    void add_row(const nlohmann::json& row);
    
    SQLRETURN fetch();
    SQLRETURN get_data(SQLUSMALLINT column_number, SQLSMALLINT target_type,
                      SQLPOINTER target_value_ptr, SQLLEN buffer_length,
                      SQLLEN* str_len_or_ind_ptr);
    
    SQLSMALLINT get_column_count() const { return static_cast<SQLSMALLINT>(columns_.size()); }
    const ColumnInfo& get_column_info(SQLUSMALLINT column_number) const;
    bool has_column(const std::string& name) const;
    SQLUSMALLINT get_column_index(const std::string& name) const;
    
    void reset();
    
    // For metadata construction
    std::vector<ColumnInfo>& get_columns() { return columns_; }
    
private:
    std::vector<ColumnInfo> columns_;
    std::vector<nlohmann::json> rows_;
    SQLULEN current_row_;
    
    void infer_schema(const std::vector<nlohmann::json>& sample_rows);
    SQLSMALLINT infer_sql_type(const nlohmann::json& value) const;
    std::string infer_type_name(SQLSMALLINT sql_type) const;
    SQLULEN infer_column_size(SQLSMALLINT sql_type) const;
    
    bool convert_value(const nlohmann::json& value, SQLSMALLINT target_type,
                      SQLPOINTER target_value_ptr, SQLLEN buffer_length,
                      SQLLEN* str_len_or_ind_ptr) const;
};

} // namespace leafodbc
