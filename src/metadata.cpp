#include "leafodbc/metadata.h"
#include "leafodbc/resultset.h"
#include "leafodbc/common.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <memory>

namespace leafodbc {

bool Metadata::matches_pattern(const std::string& value, const std::string& pattern) {
    if (pattern.empty() || pattern == "%") {
        return true;
    }
    
    // Simple wildcard matching: % = any, _ = single char
    // For now, just check if pattern is contained or exact match
    if (pattern.find('%') == std::string::npos && pattern.find('_') == std::string::npos) {
        return value == pattern;
    }
    
    // Simple contains check for MVP
    return value.find(pattern) != std::string::npos;
}

SQLINTEGER Metadata::infer_geometry_type(const std::string& wkt) {
    if (wkt.empty()) {
        return 0; // ST_Geometry generic
    }
    
    std::string upper_wkt = wkt;
    std::transform(upper_wkt.begin(), upper_wkt.end(), upper_wkt.begin(), ::toupper);
    
    if (upper_wkt.find("POINT") == 0) {
        if (upper_wkt.find("MULTIPOINT") == 0) {
            return 7; // ST_MultiPoint
        }
        return 1; // ST_Point
    } else if (upper_wkt.find("LINESTRING") == 0) {
        if (upper_wkt.find("MULTILINESTRING") == 0) {
            return 9; // ST_MultiLineString
        }
        return 3; // ST_LineString
    } else if (upper_wkt.find("POLYGON") == 0) {
        if (upper_wkt.find("MULTIPOLYGON") == 0) {
            return 11; // ST_MultiPolygon
        }
        return 5; // ST_Polygon
    }
    
    return 0; // ST_Geometry generic
}

std::unique_ptr<ResultSet> Metadata::get_tables(
    const std::string& catalog_pattern,
    const std::string& schema_pattern,
    const std::string& table_pattern,
    const std::string& type_pattern) {
    
    auto result = std::make_unique<ResultSet>();
    
    // Define columns for SQLTables result set
    ColumnInfo col_catalog;
    col_catalog.name = "TABLE_CAT";
    col_catalog.sql_type = SQL_VARCHAR;
    col_catalog.column_size = 128;
    col_catalog.nullable = SQL_NULLABLE;
    col_catalog.type_name = "VARCHAR";
    
    ColumnInfo col_schema;
    col_schema.name = "TABLE_SCHEM";
    col_schema.sql_type = SQL_VARCHAR;
    col_schema.column_size = 128;
    col_schema.nullable = SQL_NULLABLE;
    col_schema.type_name = "VARCHAR";
    
    ColumnInfo col_name;
    col_name.name = "TABLE_NAME";
    col_name.sql_type = SQL_VARCHAR;
    col_name.column_size = 128;
    col_name.nullable = SQL_NULLABLE;
    col_name.type_name = "VARCHAR";
    
    ColumnInfo col_type;
    col_type.name = "TABLE_TYPE";
    col_type.sql_type = SQL_VARCHAR;
    col_type.column_size = 128;
    col_type.nullable = SQL_NULLABLE;
    col_type.type_name = "VARCHAR";
    
    ColumnInfo col_remarks;
    col_remarks.name = "REMARKS";
    col_remarks.sql_type = SQL_VARCHAR;
    col_remarks.column_size = 255;
    col_remarks.nullable = SQL_NULLABLE;
    col_remarks.type_name = "VARCHAR";
    
    result->get_columns() = {col_catalog, col_schema, col_name, col_type, col_remarks};
    
    // Add "points" table
    if (matches_pattern("leaf", catalog_pattern) &&
        matches_pattern("pointlake", schema_pattern) &&
        matches_pattern("points", table_pattern) &&
        (type_pattern.empty() || type_pattern == "%" || type_pattern == "TABLE")) {
        
        nlohmann::json row;
        row["TABLE_CAT"] = "leaf";
        row["TABLE_SCHEM"] = "pointlake";
        row["TABLE_NAME"] = "points";
        row["TABLE_TYPE"] = "TABLE";
        row["REMARKS"] = "";
        result->add_row(row);
    }
    
    // Add "GEOMETRY_COLUMNS" table
    if (matches_pattern("leaf", catalog_pattern) &&
        (schema_pattern.empty() || schema_pattern == "%" || matches_pattern("public", schema_pattern) || matches_pattern("leaf", schema_pattern)) &&
        matches_pattern("GEOMETRY_COLUMNS", table_pattern) &&
        (type_pattern.empty() || type_pattern == "%" || type_pattern == "TABLE")) {
        
        nlohmann::json row;
        row["TABLE_CAT"] = "leaf";
        row["TABLE_SCHEM"] = "public";
        row["TABLE_NAME"] = "GEOMETRY_COLUMNS";
        row["TABLE_TYPE"] = "TABLE";
        row["REMARKS"] = "";
        result->add_row(row);
    }
    
    return result;
}

std::unique_ptr<ResultSet> Metadata::get_columns(
    const std::string& catalog_pattern,
    const std::string& schema_pattern,
    const std::string& table_pattern,
    const std::string& column_pattern) {
    
    auto result = std::make_unique<ResultSet>();
    
    // Define columns for SQLColumns result set
    ColumnInfo col_catalog;
    col_catalog.name = "TABLE_CAT";
    col_catalog.sql_type = SQL_VARCHAR;
    col_catalog.column_size = 128;
    col_catalog.nullable = SQL_NULLABLE;
    col_catalog.type_name = "VARCHAR";
    
    ColumnInfo col_schema;
    col_schema.name = "TABLE_SCHEM";
    col_schema.sql_type = SQL_VARCHAR;
    col_schema.column_size = 128;
    col_schema.nullable = SQL_NULLABLE;
    col_schema.type_name = "VARCHAR";
    
    ColumnInfo col_name;
    col_name.name = "TABLE_NAME";
    col_name.sql_type = SQL_VARCHAR;
    col_name.column_size = 128;
    col_name.nullable = SQL_NULLABLE;
    col_name.type_name = "VARCHAR";
    
    ColumnInfo col_colname;
    col_colname.name = "COLUMN_NAME";
    col_colname.sql_type = SQL_VARCHAR;
    col_colname.column_size = 128;
    col_colname.nullable = SQL_NULLABLE;
    col_colname.type_name = "VARCHAR";
    
    ColumnInfo col_datatype;
    col_datatype.name = "DATA_TYPE";
    col_datatype.sql_type = SQL_INTEGER;
    col_datatype.column_size = 0;
    col_datatype.nullable = SQL_NO_NULLS;
    col_datatype.type_name = "INTEGER";
    
    ColumnInfo col_typename;
    col_typename.name = "TYPE_NAME";
    col_typename.sql_type = SQL_VARCHAR;
    col_typename.column_size = 128;
    col_typename.nullable = SQL_NULLABLE;
    col_typename.type_name = "VARCHAR";
    
    ColumnInfo col_colsize;
    col_colsize.name = "COLUMN_SIZE";
    col_colsize.sql_type = SQL_INTEGER;
    col_colsize.column_size = 0;
    col_colsize.nullable = SQL_NULLABLE;
    col_colsize.type_name = "INTEGER";
    
    ColumnInfo col_buflen;
    col_buflen.name = "BUFFER_LENGTH";
    col_buflen.sql_type = SQL_INTEGER;
    col_buflen.column_size = 0;
    col_buflen.nullable = SQL_NULLABLE;
    col_buflen.type_name = "INTEGER";
    
    ColumnInfo col_decdigits;
    col_decdigits.name = "DECIMAL_DIGITS";
    col_decdigits.sql_type = SQL_INTEGER;
    col_decdigits.column_size = 0;
    col_decdigits.nullable = SQL_NULLABLE;
    col_decdigits.type_name = "INTEGER";
    
    ColumnInfo col_numprec;
    col_numprec.name = "NUM_PREC_RADIX";
    col_numprec.sql_type = SQL_INTEGER;
    col_numprec.column_size = 0;
    col_numprec.nullable = SQL_NULLABLE;
    col_numprec.type_name = "INTEGER";
    
    ColumnInfo col_nullable;
    col_nullable.name = "NULLABLE";
    col_nullable.sql_type = SQL_INTEGER;
    col_nullable.column_size = 0;
    col_nullable.nullable = SQL_NO_NULLS;
    col_nullable.type_name = "INTEGER";
    
    ColumnInfo col_remarks;
    col_remarks.name = "REMARKS";
    col_remarks.sql_type = SQL_VARCHAR;
    col_remarks.column_size = 255;
    col_remarks.nullable = SQL_NULLABLE;
    col_remarks.type_name = "VARCHAR";
    
    result->get_columns() = {col_catalog, col_schema, col_name, col_colname, col_datatype,
                       col_typename, col_colsize, col_buflen, col_decdigits, col_numprec,
                       col_nullable, col_remarks};
    
    // Handle "points" table
    if (matches_pattern("leaf", catalog_pattern) &&
        matches_pattern("pointlake", schema_pattern) &&
        matches_pattern("points", table_pattern)) {
        
        // Define known columns for "points" table
        struct PointColumn {
            const char* name;
            SQLSMALLINT sql_type;
            const char* type_name;
            SQLULEN column_size;
            SQLSMALLINT nullable;
        };
        
        PointColumn point_columns[] = {
            {"geometry", SQL_LONGVARCHAR, "LONGVARCHAR", 0, SQL_NULLABLE},
            {"timestamp", SQL_VARCHAR, "VARCHAR", 255, SQL_NULLABLE},
            {"operationType", SQL_VARCHAR, "VARCHAR", 255, SQL_NULLABLE},
            {"apiOwnerUsername", SQL_VARCHAR, "VARCHAR", 255, SQL_NULLABLE},
            {"crop", SQL_VARCHAR, "VARCHAR", 255, SQL_NULLABLE},
            {"feature_count", SQL_BIGINT, "BIGINT", 19, SQL_NULLABLE},
            {"fileId", SQL_VARCHAR, "VARCHAR", 255, SQL_NULLABLE}
        };
        
        for (const auto& col_def : point_columns) {
            if (matches_pattern(col_def.name, column_pattern)) {
                nlohmann::json row;
                row["TABLE_CAT"] = "leaf";
                row["TABLE_SCHEM"] = "pointlake";
                row["TABLE_NAME"] = "points";
                row["COLUMN_NAME"] = col_def.name;
                row["DATA_TYPE"] = col_def.sql_type;
                row["TYPE_NAME"] = col_def.type_name;
                row["COLUMN_SIZE"] = col_def.column_size;
                row["BUFFER_LENGTH"] = col_def.column_size;
                row["DECIMAL_DIGITS"] = 0;
                row["NUM_PREC_RADIX"] = 10;
                row["NULLABLE"] = col_def.nullable;
                row["REMARKS"] = "";
                result->add_row(row);
            }
        }
    }
    
    // Handle "GEOMETRY_COLUMNS" table
    if (is_geometry_columns_table(
        catalog_pattern.empty() ? "leaf" : catalog_pattern,
        schema_pattern.empty() ? "public" : schema_pattern,
        table_pattern)) {
        
        struct GeoColumn {
            const char* name;
            SQLSMALLINT sql_type;
            const char* type_name;
            SQLULEN column_size;
        };
        
        GeoColumn geo_columns[] = {
            {"F_TABLE_CATALOG", SQL_VARCHAR, "VARCHAR", 128},
            {"F_TABLE_SCHEMA", SQL_VARCHAR, "VARCHAR", 128},
            {"F_TABLE_NAME", SQL_VARCHAR, "VARCHAR", 128},
            {"F_GEOMETRY_COLUMN", SQL_VARCHAR, "VARCHAR", 128},
            {"GEOMETRY_TYPE", SQL_INTEGER, "INTEGER", 0},
            {"SRID", SQL_INTEGER, "INTEGER", 0}
        };
        
        for (const auto& col_def : geo_columns) {
            if (matches_pattern(col_def.name, column_pattern)) {
                nlohmann::json row;
                row["TABLE_CAT"] = "leaf";
                row["TABLE_SCHEM"] = "public";
                row["TABLE_NAME"] = "GEOMETRY_COLUMNS";
                row["COLUMN_NAME"] = col_def.name;
                row["DATA_TYPE"] = col_def.sql_type;
                row["TYPE_NAME"] = col_def.type_name;
                row["COLUMN_SIZE"] = col_def.column_size;
                row["BUFFER_LENGTH"] = col_def.column_size;
                row["DECIMAL_DIGITS"] = 0;
                row["NUM_PREC_RADIX"] = 10;
                row["NULLABLE"] = SQL_NULLABLE;
                row["REMARKS"] = "";
                result->add_row(row);
            }
        }
    }
    
    return result;
}

std::unique_ptr<ResultSet> Metadata::get_geometry_columns() {
    auto result = std::make_unique<ResultSet>();
    
    // Define columns
    ColumnInfo col_catalog;
    col_catalog.name = "F_TABLE_CATALOG";
    col_catalog.sql_type = SQL_VARCHAR;
    col_catalog.column_size = 128;
    col_catalog.nullable = SQL_NULLABLE;
    col_catalog.type_name = "VARCHAR";
    
    ColumnInfo col_schema;
    col_schema.name = "F_TABLE_SCHEMA";
    col_schema.sql_type = SQL_VARCHAR;
    col_schema.column_size = 128;
    col_schema.nullable = SQL_NULLABLE;
    col_schema.type_name = "VARCHAR";
    
    ColumnInfo col_table;
    col_table.name = "F_TABLE_NAME";
    col_table.sql_type = SQL_VARCHAR;
    col_table.column_size = 128;
    col_table.nullable = SQL_NULLABLE;
    col_table.type_name = "VARCHAR";
    
    ColumnInfo col_geom_col;
    col_geom_col.name = "F_GEOMETRY_COLUMN";
    col_geom_col.sql_type = SQL_VARCHAR;
    col_geom_col.column_size = 128;
    col_geom_col.nullable = SQL_NULLABLE;
    col_geom_col.type_name = "VARCHAR";
    
    ColumnInfo col_geom_type;
    col_geom_type.name = "GEOMETRY_TYPE";
    col_geom_type.sql_type = SQL_INTEGER;
    col_geom_type.column_size = 0;
    col_geom_type.nullable = SQL_NULLABLE;
    col_geom_type.type_name = "INTEGER";
    
    ColumnInfo col_srid;
    col_srid.name = "SRID";
    col_srid.sql_type = SQL_INTEGER;
    col_srid.column_size = 0;
    col_srid.nullable = SQL_NULLABLE;
    col_srid.type_name = "INTEGER";
    
    result->get_columns() = {col_catalog, col_schema, col_table, col_geom_col, col_geom_type, col_srid};
    
    // Add row for "points" table
    nlohmann::json row;
    row["F_TABLE_CATALOG"] = "leaf";
    row["F_TABLE_SCHEMA"] = "pointlake";
    row["F_TABLE_NAME"] = "points";
    row["F_GEOMETRY_COLUMN"] = "geometry";
    row["GEOMETRY_TYPE"] = 0; // ST_Geometry generic (can be inferred from WKT later)
    row["SRID"] = 4326; // Default assumption (WGS84)
    result->add_row(row);
    
    return result;
}

bool Metadata::is_geometry_columns_table(const std::string& catalog,
                                         const std::string& schema,
                                         const std::string& table) {
    std::string upper_table = table;
    std::transform(upper_table.begin(), upper_table.end(), upper_table.begin(), ::toupper);
    return upper_table == "GEOMETRY_COLUMNS";
}

} // namespace leafodbc
