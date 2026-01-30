# LeafODBC Driver

ODBC driver for accessing the Leaf PointLake API, enabling tools like QGIS, GDAL/OGR, and other ODBC applications to execute SQL queries against the Leaf PointLake API.

## Features

- ✅ **Read-only**: Supports SELECT only (blocks DDL/DML)
- ✅ **Authentication**: Automatic authentication via Leaf API
- ✅ **Metadata**: Exposes tables and columns for GDAL/OGR
- ✅ **Geometry**: WKT geometry support through `GEOMETRY_COLUMNS`
- ✅ **Compatible**: Works with unixODBC (macOS and Linux)
- ✅ **QGIS**: Full integration with QGIS via OGR/GDAL

## Architecture

- **Language**: C++17
- **Build**: CMake
- **Dependencies**:
  - unixODBC (headers and libraries)
  - libcurl (HTTP client)
  - nlohmann/json (JSON parsing)

## Installation

### Option 1: Download Pre-built Release (Recommended)

1. **Download the release** for your platform:
   - Go to [Releases](https://github.com/lhzsantana/leaf-odbc-driver/releases)
   - Download `leafodbc-linux.tar.gz` (Linux) or `leafodbc-macos.tar.gz` (macOS)

2. **Extract the archive**:
   ```bash
   tar -xzf leafodbc-macos.tar.gz
   cd leafodbc-macos
   ```

3. **Install the driver library**:
   
   **macOS:**
   ```bash
   # Copy to a system or user library directory
   sudo cp libleafodbc.dylib /usr/local/lib/
   # Or user-specific:
   mkdir -p ~/lib
   cp libleafodbc.dylib ~/lib/
   ```
   
   **Linux:**
   ```bash
   # Copy to system library directory
   sudo cp libleafodbc.so /usr/local/lib/
   # Or user-specific:
   mkdir -p ~/lib
   cp libleafodbc.so ~/lib/
   ```

4. **Register the driver** with unixODBC (see [ODBC Setup](#odbc-setup) below)

### Option 2: Build from Source

```bash
git clone https://github.com/lhzsantana/leaf-odbc-driver.git
cd leaf-odbc-driver
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

The driver will be generated as `build/lib/libleafodbc.so` (Linux) or `build/lib/libleafodbc.dylib` (macOS).

## ODBC Setup

### Step 1: Register the Driver

Edit `odbcinst.ini` (usually `/etc/odbcinst.ini`, `/usr/local/etc/odbcinst.ini`, or `~/.odbcinst.ini`):

**macOS:**
```ini
[LeafODBC]
Description=Leaf ODBC Driver for PointLake API
Driver=/usr/local/lib/libleafodbc.dylib
# Or if installed in user directory:
# Driver=/Users/your_username/lib/libleafodbc.dylib
```

**Linux:**
```ini
[LeafODBC]
Description=Leaf ODBC Driver for PointLake API
Driver=/usr/local/lib/libleafodbc.so
# Or if installed in user directory:
# Driver=/home/your_username/lib/libleafodbc.so
```

**Verify registration:**
```bash
odbcinst -q -d
# Should show: LeafODBC
```

### Step 2: Create Data Source (DSN)

Edit `odbc.ini` (usually `~/.odbc.ini` or `/etc/odbc.ini`):

```ini
[LeafPointLake]
Driver=LeafODBC
EndpointBase=https://api.withleaf.io
SqlEngine=SPARK_SQL
Username=YOUR_USERNAME_HERE
Password=YOUR_PASSWORD_HERE
RememberMe=true
TimeoutSec=60
VerifyTLS=true
UserAgent=LeafODBC/0.1
```

**Verify DSN:**
```bash
odbcinst -q -s
# Should show: LeafPointLake
```

### Step 3: Test Connection

```bash
isql -v LeafPointLake
# If successful, you'll see:
# +---------------------------------------+
# | Connected!                            |
# |                                       |
# | sql-statement                         |
# | help [tablename]                      |
# | quit                                  |
# +---------------------------------------+
# SQL>
```

## Using with QGIS

### Prerequisites

1. **QGIS installed** (version 3.x or later recommended)
2. **GDAL/OGR with ODBC support** (usually included with QGIS)
3. **LeafODBC driver installed and registered** (see [Installation](#installation) above)

### Verify ODBC Support in GDAL

```bash
ogrinfo --formats | grep -i odbc
# Should show: ODBC -vector- (read/write)
```

If ODBC doesn't appear, you may need to install GDAL with ODBC support:
- **macOS**: `brew install gdal`
- **Linux**: `sudo apt-get install gdal-bin` (Ubuntu/Debian)

### Method 1: Add Layer via Data Source Manager (Recommended)

1. **Open QGIS**

2. **Open Data Source Manager**:
   - Go to **Layer → Data Source Manager** (or press `Ctrl+L` / `Cmd+L`)

3. **Select Vector Layer**:
   - Click the **Vector** tab
   - Under **Source Type**, select **Database → ODBC**

4. **Configure Connection**:
   - **Connection string**: `ODBC:LeafPointLake`
     - Or with explicit credentials: `ODBC:Username=your_user;Password=your_pass@LeafPointLake`
   - **SQL** (optional): Add a query to filter data
     ```
     SELECT geometry, timestamp, operationType 
     FROM leaf.pointlake.points 
     LIMIT 1000
     ```

5. **Add Layer**:
   - Click **Add**
   - The layer should appear in your map

### Method 2: Add Layer via Browser Panel

1. In the **Browser** panel (usually on the left), expand **ODBC**
2. Expand your DSN (`LeafPointLake`)
3. Expand **leaf → pointlake**
4. Drag the **points** table to the map canvas

### Method 3: Add Layer via Python Console

Open QGIS Python Console (`Plugins → Python Console`) and run:

```python
from qgis.core import QgsVectorLayer, QgsProject

# Simple connection via DSN
uri = "ODBC:LeafPointLake"
layer = QgsVectorLayer(uri, "Leaf Points", "ogr")

# Or with SQL query
uri = "ODBC:LeafPointLake|layername=points|sql=SELECT geometry, timestamp FROM leaf.pointlake.points LIMIT 100"
layer = QgsVectorLayer(uri, "Leaf Points Filtered", "ogr")

if layer.isValid():
    # Set CRS (WGS84)
    layer.setCrs(QgsCoordinateReferenceSystem("EPSG:4326"))
    
    # Add to project
    QgsProject.instance().addMapLayer(layer)
    
    # Zoom to layer
    iface.mapCanvas().setExtent(layer.extent())
    iface.mapCanvas().refresh()
    
    print(f"✓ Layer added: {layer.featureCount()} features")
else:
    print(f"✗ Error: {layer.error().message()}")
```

### Configure Geometry Column

QGIS should automatically detect the `geometry` column through the `GEOMETRY_COLUMNS` table. If it doesn't:

1. Right-click the layer → **Properties**
2. Go to **Source** tab
3. Under **Geometry column**, select `geometry`
4. Under **Geometry type**, choose:
   - **Point** (for POINT geometries)
   - **LineString** (for LINESTRING)
   - **Polygon** (for POLYGON)
   - **Auto** (let QGIS detect automatically)

### Set Coordinate Reference System (CRS)

The driver assumes SRID 4326 (WGS84) by default:

1. Right-click layer → **Properties → Source**
2. Under **CRS**, select **EPSG:4326** (WGS84) or your appropriate CRS
3. Click **Apply**

## Connection Parameters

All parameters can be set in the DSN (`odbc.ini`) or via connection string:

- `EndpointBase`: API base URL (default: `https://api.withleaf.io`)
- `Username`: Leaf API username
- `Password`: Leaf API password
- `RememberMe`: `true`/`false` (default: `true`)
- `SqlEngine`: SQL engine (default: `SPARK_SQL`)
- `TimeoutSec`: Timeout in seconds (default: `60`)
- `VerifyTLS`: Verify TLS certificates (default: `true`)
- `UserAgent`: HTTP user agent (default: `LeafODBC/0.1`)

## Exposed Tables

### `leaf.pointlake.points`

Main table with point data. Known columns:
- `geometry` (LONGVARCHAR, WKT format)
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

## Usage Examples

### Via isql (Command Line)

```bash
isql -v LeafPointLake
SQL> SELECT geometry, timestamp FROM leaf.pointlake.points LIMIT 10;
SQL> SELECT * FROM GEOMETRY_COLUMNS;
SQL> quit
```

### Via ogrinfo (GDAL)

```bash
# List all layers
ogrinfo "ODBC:LeafPointLake" -al

# Execute SQL query
ogrinfo "ODBC:LeafPointLake" -sql "SELECT geometry, timestamp FROM leaf.pointlake.points LIMIT 100"

# Get layer info
ogrinfo "ODBC:LeafPointLake" points -so
```

### Via QGIS SQL Query

Use **Layer → Add Layer → Add/Edit Virtual Layer**:

```sql
SELECT 
    geometry,
    timestamp,
    operationType,
    crop
FROM leaf.pointlake.points
WHERE timestamp >= '2024-01-01'
LIMIT 1000
```

## Troubleshooting

### Driver Not Found

**Error**: `Driver not found` or `Could not load driver`

**Solutions**:
1. Verify driver is registered: `odbcinst -q -d | grep LeafODBC`
2. Check driver path in `odbcinst.ini` is correct and absolute
3. Verify file exists and has read permissions: `ls -l /path/to/libleafodbc.*`
4. Check library dependencies: `ldd libleafodbc.so` (Linux) or `otool -L libleafodbc.dylib` (macOS)

### Connection Failed in QGIS

**Error**: `Layer is not valid` or connection fails

**Solutions**:
1. Test connection with `isql` first: `isql -v LeafPointLake`
2. Verify DSN exists: `odbcinst -q -s | grep LeafPointLake`
3. Check credentials in `odbc.ini`
4. Enable debug logs: `export LEAFODBC_LOG=1` before starting QGIS
5. Check QGIS logs: **View → Panels → Log Messages**

### No Geometry Column Found

**Error**: QGIS doesn't detect geometry column

**Solutions**:
1. Verify `GEOMETRY_COLUMNS` table is accessible:
   ```bash
   isql -v LeafPointLake -b
   SQL> SELECT * FROM GEOMETRY_COLUMNS;
   ```
2. Manually configure geometry column in layer properties
3. Ensure geometry data is in WKT format

### Authentication Failed

**Error**: `Authentication failed` or `28000` error

**Solutions**:
1. Verify `Username` and `Password` in `odbc.ini`
2. Test authentication with `isql`
3. Check `EndpointBase` is correct
4. Use connection string with explicit credentials in QGIS

### Performance Issues

**Problem**: QGIS is slow or hangs when loading data

**Solutions**:
1. Always use `LIMIT` in queries (e.g., `LIMIT 1000`)
2. Use `TABLESAMPLE` for large datasets:
   ```sql
   SELECT * FROM leaf.pointlake.points TABLESAMPLE (1 PERCENT) LIMIT 5000
   ```
3. Filter data with WHERE clauses before loading
4. Consider creating virtual layers with pre-filtered queries

### ODBC Not Available in QGIS

**Problem**: ODBC option doesn't appear in QGIS

**Solutions**:
1. Verify GDAL has ODBC support: `ogrinfo --formats | grep ODBC`
2. Install GDAL with ODBC support:
   - macOS: `brew install gdal`
   - Linux: `sudo apt-get install gdal-bin libgdal-dev`
3. Restart QGIS after installing GDAL

## Debug Logging

Enable detailed logs for troubleshooting:

```bash
export LEAFODBC_LOG=1
# Then run your application (isql, ogrinfo, or QGIS)
isql -v LeafPointLake
```

Logs will appear in `stderr` and show:
- Authentication attempts
- Query execution
- HTTP status codes
- Error messages

**Note**: Passwords and tokens are never logged for security.

## Limitations (MVP)

- ✅ SELECT only (read-only) - INSERT, UPDATE, DELETE, DROP, CREATE are blocked
- ✅ Basic data type support
- ✅ Schema inference from sample rows
- ✅ Fixed SRID at 4326 (configurable manually in QGIS)
- ⚠️ Windows not supported yet (macOS/Linux only)
- ⚠️ No connection pooling
- ⚠️ No prepared statement caching

## Documentation

- **[ODBC Setup Guide](docs/ODBC_SETUP.md)** - Detailed unixODBC configuration
- **[QGIS Setup Guide](docs/QGIS_SETUP.md)** - Complete QGIS integration guide
- **[Build Guide](BUILD.md)** - Compilation instructions

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

### Build from Source

```bash
git clone https://github.com/lhzsantana/leaf-odbc-driver.git
cd leaf-odbc-driver
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## License

[Add your license here]

## Contributing

[Add contribution information]

## Support

For issues and questions:
1. Check logs with `LEAFODBC_LOG=1`
2. Test connection with `isql -v LeafPointLake`
3. Verify unixODBC configuration: `odbcinst -j`
4. Check [Troubleshooting](#troubleshooting) section above
5. Open an issue on [GitHub](https://github.com/lhzsantana/leaf-odbc-driver/issues)
