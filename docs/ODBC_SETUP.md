# LeafODBC Driver Configuration

This document describes how to configure and use the LeafODBC driver on macOS and Linux with unixODBC.

## Prerequisites

- unixODBC installed
- libcurl installed
- CMake 3.15 or higher
- C++17 compiler (clang/gcc)

### Installing Dependencies

**macOS (Homebrew):**
```bash
brew install unixodbc curl cmake
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install unixodbc-dev libcurl4-openssl-dev cmake build-essential
```

**Linux (RHEL/CentOS):**
```bash
sudo yum install unixODBC-devel libcurl-devel cmake gcc-c++
```

## Compilation

```bash
cd leaf-odbc-driver
mkdir build
cd build
cmake ..
make
```

The driver will be compiled as:
- macOS: `build/lib/libleafodbc.dylib`
- Linux: `build/lib/libleafodbc.so`

## Driver Registration in unixODBC

### 1. Locate odbcinst.ini

The `odbcinst.ini` file is usually located at:
- macOS: `/usr/local/etc/odbcinst.ini` or `/opt/homebrew/etc/odbcinst.ini`
- Linux: `/etc/odbcinst.ini` or `/usr/local/etc/odbcinst.ini`

### 2. Add Driver Entry

Edit the `odbcinst.ini` file (may require `sudo`) and add:

```ini
[LeafODBC]
Description=Leaf ODBC Driver for PointLake API
Driver=/absolute/path/to/build/lib/libleafodbc.so
# or .dylib on macOS:
# Driver=/absolute/path/to/build/lib/libleafodbc.dylib
```

**macOS Example:**
```ini
[LeafODBC]
Description=Leaf ODBC Driver for PointLake API
Driver=/Users/your_user/leaf-odbc-driver/build/lib/libleafodbc.dylib
```

**Linux Example:**
```ini
[LeafODBC]
Description=Leaf ODBC Driver for PointLake API
Driver=/home/your_user/leaf-odbc-driver/build/lib/libleafodbc.so
```

### 3. Verify Registration

```bash
odbcinst -q -d
```

You should see `LeafODBC` in the driver list.

## DSN (Data Source Name) Configuration

### 1. Locate odbc.ini

The `odbc.ini` file is usually located at:
- macOS: `~/.odbc.ini` or `/usr/local/etc/odbc.ini`
- Linux: `~/.odbc.ini` or `/etc/odbc.ini`

### 2. Create DSN

Edit the `odbc.ini` file and add a section:

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

**Parameters:**
- `Driver`: Registered driver name (must be `LeafODBC`)
- `EndpointBase`: API base URL (default: `https://api.withleaf.io`)
- `SqlEngine`: SQL engine to use (default: `SPARK_SQL`)
- `Username`: Your Leaf API username
- `Password`: Your Leaf API password
- `RememberMe`: `true` or `false` (default: `true`)
- `TimeoutSec`: Timeout in seconds (default: `60`)
- `VerifyTLS`: Verify TLS certificates (default: `true`)
- `UserAgent`: HTTP user agent (default: `LeafODBC/0.1`)

### 3. Verify DSN

```bash
odbcinst -q -s
```

You should see `LeafPointLake` in the DSN list.

## Testing with isql

The `isql` tool is a command-line utility from unixODBC to test ODBC connections.

### Connect

```bash
isql -v LeafPointLake
```

If connected successfully, you will see:
```
+---------------------------------------+
| Connected!                            |
|                                       |
| sql-statement                         |
| help [tablename]                      |
| quit                                  |
|                                       |
+---------------------------------------+
SQL>
```

### Execute Queries

```sql
-- Simple test
SELECT 1;

-- List tables
SELECT * FROM leaf.pointlake.points LIMIT 10;

-- Query GEOMETRY_COLUMNS
SELECT * FROM GEOMETRY_COLUMNS;

-- Query with geometry
SELECT geometry, timestamp, operationType 
FROM leaf.pointlake.points 
TABLESAMPLE (0.1 PERCENT) 
LIMIT 10;
```

### Exit

```sql
quit
```

## Testing with ogrinfo (GDAL)

The `ogrinfo` tool is a GDAL utility for inspecting spatial data sources.

### List Layers

```bash
ogrinfo "ODBC:LeafPointLake" -al
```

### Execute SQL Query

```bash
ogrinfo "ODBC:LeafPointLake" -sql "SELECT geometry, timestamp FROM leaf.pointlake.points LIMIT 10"
```

### Use Username/Password in Connection String

```bash
ogrinfo "ODBC:Username=your_user;Password=your_pass@LeafPointLake" -al
```

## Troubleshooting

### Error: "Driver not found"

- Verify the driver path in `odbcinst.ini` is correct and absolute
- Verify the `.so` or `.dylib` file exists and has read permissions
- Run `odbcinst -q -d` to verify the driver is registered

### Error: "Authentication failed"

- Verify `Username` and `Password` are correct in `odbc.ini`
- Verify `EndpointBase` is correct
- Enable logs with `export LEAFODBC_LOG=1` and try again

### Error: "Only SELECT statements are allowed"

- The driver is read-only and only allows SELECT
- Verify you're not trying to execute INSERT, UPDATE, DELETE, etc.

### Enable Debug Logs

```bash
export LEAFODBC_LOG=1
isql -v LeafPointLake
```

Logs will appear in `stderr`.

### Check ODBC Environment Variables

```bash
odbcinst -j
```

This shows the paths that unixODBC is using.

## Direct Connection String

You can also connect using a connection string directly (without DSN):

```bash
isql -v "Driver=LeafODBC;EndpointBase=https://api.withleaf.io;Username=user;Password=pass;SqlEngine=SPARK_SQL"
```

The connection string overrides DSN parameters if both are specified.
