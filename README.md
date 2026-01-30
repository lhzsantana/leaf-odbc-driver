# LeafODBC Driver

ODBC driver for accessing the Leaf PointLake API, enabling tools like QGIS, GDAL/OGR, and other ODBC applications to execute SQL queries against the Leaf PointLake API.

## Features

- ✅ **Read-only**: Supports SELECT only (blocks DDL/DML)
- ✅ **Authentication**: Automatic authentication via Leaf API
- ✅ **Metadata**: Exposes tables and columns for GDAL/OGR
- ✅ **Geometry**: WKT geometry support through `GEOMETRY_COLUMNS`
- ✅ **Compatible**: Works with unixODBC (macOS and Linux)
- ✅ **QGIS**: Full integration with QGIS via OGR

## Architecture

- **Language**: C++17
- **Build**: CMake
- **Dependencies**:
  - unixODBC (headers and libraries)
  - libcurl (HTTP client)
  - nlohmann/json (JSON parsing)

## Quick Build

```bash
mkdir build && cd build
cmake ..
make
```

The driver will be generated as `build/lib/libleafodbc.so` (Linux) or `build/lib/libleafodbc.dylib` (macOS).

## Quick Setup

1. **Register driver** in `odbcinst.ini`:
   ```ini
   [LeafODBC]
   Description=Leaf ODBC Driver
   Driver=/absolute/path/to/libleafodbc.so
   ```

2. **Create DSN** in `odbc.ini`:
   ```ini
   [LeafPointLake]
   Driver=LeafODBC
   EndpointBase=https://api.withleaf.io
   Username=YOUR_USER
   Password=YOUR_PASSWORD
   SqlEngine=SPARK_SQL
   ```

3. **Test**:
   ```bash
   isql -v LeafPointLake
   ```

## Documentation

- [ODBC Setup](docs/ODBC_SETUP.md) - Complete unixODBC setup
- [QGIS Setup](docs/QGIS_SETUP.md) - How to use with QGIS

## Basic Usage

### Via isql

```bash
isql -v LeafPointLake
SQL> SELECT geometry, timestamp FROM leaf.pointlake.points LIMIT 10;
```

### Via ogrinfo (GDAL)

```bash
ogrinfo "ODBC:LeafPointLake" -al
ogrinfo "ODBC:LeafPointLake" -sql "SELECT * FROM leaf.pointlake.points LIMIT 100"
```

### Via QGIS

1. **Layer → Data Source Manager → Vector → ODBC**
2. Connection string: `ODBC:LeafPointLake`
3. Add layer `points`

## Connection Parameters

- `EndpointBase`: API base URL (default: `https://api.withleaf.io`)
- `Username`: Leaf API username
- `Password`: Leaf API password
- `RememberMe`: `true`/`false` (default: `true`)
- `SqlEngine`: SQL engine (default: `SPARK_SQL`)
- `TimeoutSec`: Timeout in seconds (default: `60`)
- `VerifyTLS`: Verify TLS (default: `true`)
- `UserAgent`: HTTP user agent (default: `LeafODBC/0.1`)

## Exposed Tables

### `leaf.pointlake.points`

Main table with point data. Known columns:
- `geometry` (LONGVARCHAR, WKT)
- `timestamp` (VARCHAR)
- `operationType` (VARCHAR)
- `apiOwnerUsername` (VARCHAR)
- `crop` (VARCHAR)
- `feature_count` (BIGINT)
- `fileId` (VARCHAR)

### `GEOMETRY_COLUMNS`

Virtual table of spatial metadata for GDAL/OGR:
- `F_TABLE_CATALOG`: "leaf"
- `F_TABLE_SCHEMA`: "pointlake"
- `F_TABLE_NAME`: "points"
- `F_GEOMETRY_COLUMN`: "geometry"
- `GEOMETRY_TYPE`: 0 (generic, can be inferred from WKT)
- `SRID`: 4326 (default WGS84)

## Debug Logging

Enable detailed logs:

```bash
export LEAFODBC_LOG=1
isql -v LeafPointLake
```

## Limitations (MVP)

- ✅ SELECT only (read-only)
- ✅ Basic data type support
- ✅ Schema inference from samples
- ✅ Fixed SRID at 4326 (configurable manually in QGIS)
- ⚠️ Windows not supported yet (macOS/Linux only)

## Development

### Project Structure

```
leaf-odbc-driver/
├── CMakeLists.txt
├── include/leafodbc/
│   ├── common.h
│   ├── handles.h
│   ├── conn_string.h
│   ├── leaf_client.h
│   ├── resultset.h
│   ├── metadata.h
│   └── sql_guard.h
├── src/
│   ├── odbc_api.cpp      # ODBC entry points
│   ├── handles.cpp       # Handle management
│   ├── conn_string.cpp   # Connection string parser
│   ├── leaf_client.cpp   # HTTP client
│   ├── resultset.cpp     # Result set and type conversion
│   ├── metadata.cpp      # Metadata (SQLTables, SQLColumns)
│   └── sql_guard.cpp     # SQL validation (read-only)
└── docs/
    ├── ODBC_SETUP.md
    └── QGIS_SETUP.md
```

### Build with Tests

```bash
cmake -DBUILD_TESTS=ON ..
make
```

## License

[Add your license here]

## Contributing

[Add contribution information]

## Support

For issues and questions:
1. Check logs with `LEAFODBC_LOG=1`
2. Test connection with `isql`
3. Check unixODBC configuration with `odbcinst -j`
