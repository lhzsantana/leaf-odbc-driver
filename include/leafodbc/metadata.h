#pragma once

#include "common.h"
#include "resultset.h"
#include <sql.h>
#include <string>

namespace leafodbc {

class Metadata {
public:
    // SQLTables: list tables
    static std::unique_ptr<ResultSet> get_tables(
        const std::string& catalog_pattern,
        const std::string& schema_pattern,
        const std::string& table_pattern,
        const std::string& type_pattern
    );
    
    // SQLColumns: describe columns
    static std::unique_ptr<ResultSet> get_columns(
        const std::string& catalog_pattern,
        const std::string& schema_pattern,
        const std::string& table_pattern,
        const std::string& column_pattern
    );
    
    // GEOMETRY_COLUMNS virtual table
    static std::unique_ptr<ResultSet> get_geometry_columns();
    
    // Check if a table is GEOMETRY_COLUMNS
    static bool is_geometry_columns_table(const std::string& catalog, 
                                         const std::string& schema,
                                         const std::string& table);
    
    // Get geometry type from WKT prefix
    static SQLINTEGER infer_geometry_type(const std::string& wkt);
    
private:
    static bool matches_pattern(const std::string& value, const std::string& pattern);
};

} // namespace leafodbc
