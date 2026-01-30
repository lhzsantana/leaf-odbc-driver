# LeafODBC Configuration in QGIS

This document provides a comprehensive guide for using the LeafODBC driver in QGIS to access Leaf PointLake data.

## Prerequisites

1. **LeafODBC driver installed** (from release or compiled)
2. **Driver registered** with unixODBC (see [ODBC_SETUP.md](ODBC_SETUP.md))
3. **DSN configured** in `odbc.ini`
4. **QGIS installed** (version 3.x or later recommended)
5. **GDAL/OGR with ODBC support** (usually included with QGIS)

### Installation Checklist

- [ ] Downloaded LeafODBC release for your platform
- [ ] Extracted and installed driver library (`libleafodbc.so` or `.dylib`)
- [ ] Registered driver in `odbcinst.ini`
- [ ] Created DSN in `odbc.ini` with credentials
- [ ] Tested connection with `isql -v LeafPointLake`
- [ ] Verified GDAL has ODBC support

### Verify ODBC Support in GDAL

```bash
ogrinfo --formats | grep -i odbc
# Should show: ODBC -vector- (read/write)
```

If ODBC doesn't appear, install GDAL with ODBC support:

**macOS:**
```bash
brew install gdal
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install gdal-bin libgdal-dev
```

**Linux (RHEL/CentOS):**
```bash
sudo yum install gdal gdal-devel
```

## Installation Steps

### Step 1: Download and Install Driver

**Download from Release:**
1. Go to [GitHub Releases](https://github.com/lhzsantana/leaf-odbc-driver/releases)
2. Download `leafodbc-macos.tar.gz` (macOS) or `leafodbc-linux.tar.gz` (Linux)
3. Extract: `tar -xzf leafodbc-*.tar.gz`
4. Copy driver library to system location:
   ```bash
   # macOS
   sudo cp libleafodbc.dylib /usr/local/lib/
   
   # Linux
   sudo cp libleafodbc.so /usr/local/lib/
   ```

**Or build from source** (see [BUILD.md](../BUILD.md))

### Step 2: Register Driver with unixODBC

Edit `odbcinst.ini` (see [ODBC_SETUP.md](ODBC_SETUP.md) for details):

```ini
[LeafODBC]
Description=Leaf ODBC Driver for PointLake API
Driver=/usr/local/lib/libleafodbc.dylib  # or .so on Linux
```

Verify: `odbcinst -q -d | grep LeafODBC`

### Step 3: Create DSN

Edit `odbc.ini`:

```ini
[LeafPointLake]
Driver=LeafODBC
EndpointBase=https://api.withleaf.io
Username=YOUR_USERNAME
Password=YOUR_PASSWORD
SqlEngine=SPARK_SQL
```

Verify: `odbcinst -q -s | grep LeafPointLake`

### Step 4: Test Connection

```bash
isql -v LeafPointLake
# Should connect successfully
```

## Adding Layer in QGIS

### Method 1: Via Data Source Manager (Recommended)

1. **Open QGIS**

2. **Open Data Source Manager**:
   - Menu: **Layer → Data Source Manager**
   - Keyboard shortcut: `Ctrl+L` (Windows/Linux) or `Cmd+L` (macOS)
   - Or click the **Data Source Manager** icon in the toolbar

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
   - Click **Close** to dismiss the dialog

**Note**: If you see "Layer is not valid", check the [Troubleshooting](#troubleshooting) section below.

### Method 2: Via Browser Panel

1. **Open Browser Panel** (if not visible):
   - Menu: **View → Panels → Browser**
   - Or press `Ctrl+Shift+B` (Windows/Linux) or `Cmd+Shift+B` (macOS)

2. **Navigate to ODBC**:
   - In the **Browser** panel (usually on the left), expand **ODBC**
   - Expand your DSN (`LeafPointLake`)
   - Expand **leaf → pointlake**

3. **Add Layer**:
   - Drag the **points** table to the map canvas
   - Or right-click → **Add Layer to Project**

**Note**: This method may not work if QGIS doesn't detect ODBC connections. Use Method 1 if this fails.

### Method 3: Via Python Console

```python
from qgis.core import QgsVectorLayer

# Create layer via ODBC
uri = "ODBC:LeafPointLake"
layer = QgsVectorLayer(uri, "Leaf Points", "ogr")

# Or with SQL query
uri = "ODBC:LeafPointLake|layername=points|sql=SELECT geometry, timestamp FROM leaf.pointlake.points LIMIT 100"
layer = QgsVectorLayer(uri, "Leaf Points Filtered", "ogr")

if layer.isValid():
    QgsProject.instance().addMapLayer(layer)
    print("Layer added successfully!")
else:
    print("Error loading layer:", layer.error().message())
```

## Configure Geometry Column

QGIS should automatically detect the `geometry` column through the `GEOMETRY_COLUMNS` table. If it doesn't:

1. Right-click the layer → **Properties**
2. Go to the **Source** tab
3. Under **Geometry column**, select `geometry`
4. Under **Geometry type**, QGIS can infer automatically, or you can choose:
   - **Point** (if POINT)
   - **LineString** (if LINESTRING)
   - **Polygon** (if POLYGON)
   - **Auto** (let QGIS detect)

## SRID/CRS

The driver assumes SRID 4326 (WGS84) by default. To change:

1. **Properties → Source → CRS**
2. Select **EPSG:4326** (WGS84) or the appropriate CRS
3. Click **Apply**

## SQL Queries in QGIS

### Via DB Manager (if available)

1. Go to **Database → DB Manager**
2. Select **ODBC → LeafPointLake**
3. Use the **SQL Window** tab to execute queries

### Via Virtual Layer

1. **Layer → Add Layer → Add/Edit Virtual Layer**
2. In **Query**, type:
   ```sql
   SELECT 
       geometry,
       timestamp,
       operationType,
       crop
   FROM leaf.pointlake.points
   WHERE timestamp > '2024-01-01'
   LIMIT 1000
   ```
3. Click **Add**

## Filters and Symbology

### Filter by Attribute

1. Right-click the layer → **Filter**
2. Type a SQL expression, for example:
   ```
   "operationType" = 'harvest'
   ```
3. Click **OK**

### Style Points

1. **Properties → Symbology**
2. Choose an appropriate style:
   - **Single symbol** for all points
   - **Categorized** to group by `operationType` or `crop`
   - **Graduated** for numeric values

## Performance

### Limit Number of Features

For large data volumes, always use `LIMIT` in queries:

```sql
SELECT geometry, timestamp 
FROM leaf.pointlake.points 
TABLESAMPLE (0.1 PERCENT) 
LIMIT 10000
```

### Use TABLESAMPLE

`TABLESAMPLE` is useful for quick visualization:

```sql
SELECT * 
FROM leaf.pointlake.points 
TABLESAMPLE (1 PERCENT) 
LIMIT 5000
```

## Troubleshooting

### Error: "Layer is not valid"

**Symptoms**: QGIS shows error when trying to add layer

**Solutions**:
1. **Test ODBC connection first**:
   ```bash
   isql -v LeafPointLake
   ```
   If this fails, fix ODBC configuration before using QGIS.

2. **Verify DSN exists**:
   ```bash
   odbcinst -q -s | grep LeafPointLake
   ```

3. **Check driver registration**:
   ```bash
   odbcinst -q -d | grep LeafODBC
   ```

4. **Verify driver file exists and is readable**:
   ```bash
   ls -l /usr/local/lib/libleafodbc.*
   # Check permissions - should be readable
   ```

5. **Check QGIS logs**:
   - **View → Panels → Log Messages**
   - Look for ODBC-related errors

6. **Try explicit connection string**:
   ```
   ODBC:Username=your_user;Password=your_pass@LeafPointLake
   ```

### Error: "No geometry column found"

**Symptoms**: Layer loads but has no geometry

**Solutions**:
1. **Verify GEOMETRY_COLUMNS table**:
   ```bash
   isql -v LeafPointLake -b
   SQL> SELECT * FROM GEOMETRY_COLUMNS;
   ```
   Should show a row for the `points` table.

2. **Manually configure geometry column**:
   - Right-click layer → **Properties → Source**
   - Set **Geometry column** to `geometry`
   - Set **Geometry type** to **Point** (or Auto)

3. **Check geometry data format**:
   - Ensure geometry column contains WKT (Well-Known Text) format
   - Example: `POINT(-47.123 -23.456)`

### Error: "Authentication failed" or "28000"

**Symptoms**: Connection fails with authentication error

**Solutions**:
1. **Verify credentials in odbc.ini**:
   ```bash
   cat ~/.odbc.ini | grep -A5 LeafPointLake
   ```

2. **Test authentication**:
   ```bash
   isql -v LeafPointLake
   ```
   If this works, the issue is QGIS-specific.

3. **Use explicit credentials in connection string**:
   ```
   ODBC:Username=your_user;Password=your_pass@LeafPointLake
   ```

4. **Check EndpointBase**:
   - Verify `EndpointBase=https://api.withleaf.io` in `odbc.ini`
   - Test API endpoint manually if needed

### Error: "Driver not found" or "Could not load driver"

**Symptoms**: QGIS can't find the ODBC driver

**Solutions**:
1. **Verify driver registration**:
   ```bash
   odbcinst -q -d
   ```

2. **Check driver path**:
   - Path in `odbcinst.ini` must be absolute
   - File must exist and be readable

3. **Check library dependencies** (Linux):
   ```bash
   ldd /usr/local/lib/libleafodbc.so
   ```
   All dependencies should be found.

4. **Check library dependencies** (macOS):
   ```bash
   otool -L /usr/local/lib/libleafodbc.dylib
   ```

5. **Set library path** (if needed):
   ```bash
   export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH  # macOS
   export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH      # Linux
   ```

### ODBC Option Not Available in QGIS

**Symptoms**: ODBC doesn't appear in Data Source Manager

**Solutions**:
1. **Verify GDAL has ODBC support**:
   ```bash
   ogrinfo --formats | grep -i odbc
   ```

2. **Install GDAL with ODBC support**:
   - macOS: `brew install gdal`
   - Linux: `sudo apt-get install gdal-bin libgdal-dev`

3. **Restart QGIS** after installing GDAL

4. **Check QGIS version**:
   - QGIS 3.x should have ODBC support
   - Older versions may need manual GDAL compilation

### Slow Performance or Hanging

**Symptoms**: QGIS becomes slow or hangs when loading data

**Solutions**:
1. **Always use LIMIT**:
   ```sql
   SELECT * FROM leaf.pointlake.points LIMIT 1000
   ```

2. **Use TABLESAMPLE for large datasets**:
   ```sql
   SELECT * FROM leaf.pointlake.points TABLESAMPLE (1 PERCENT) LIMIT 5000
   ```

3. **Filter data before loading**:
   ```sql
   SELECT geometry, timestamp 
   FROM leaf.pointlake.points 
   WHERE timestamp >= '2024-01-01'
   LIMIT 1000
   ```

4. **Use Virtual Layers** for complex queries instead of loading full tables

5. **Check network connection** - slow API responses will slow QGIS

### Debug Tips

1. **Enable LeafODBC logs**:
   ```bash
   export LEAFODBC_LOG=1
   # Then start QGIS from terminal to see logs
   ```

2. **Check QGIS logs**:
   - **View → Panels → Log Messages**
   - Filter by "ODBC" or "OGR"

3. **Test with ogrinfo** (command-line GDAL):
   ```bash
   ogrinfo "ODBC:LeafPointLake" -al
   ```
   If this works but QGIS doesn't, it's a QGIS configuration issue.

4. **Verify unixODBC configuration**:
   ```bash
   odbcinst -j
   # Shows ODBC configuration paths
   ```

## Complete Example: Python Script

```python
from qgis.core import QgsVectorLayer, QgsProject

# Connection string
dsn = "LeafPointLake"
# or with credentials:
# dsn = "ODBC:Username=user;Password=pass@LeafPointLake"

# SQL query
sql = """
SELECT 
    geometry,
    timestamp,
    operationType,
    crop,
    apiOwnerUsername
FROM leaf.pointlake.points
WHERE timestamp >= '2024-01-01'
LIMIT 5000
"""

# Create URI
uri = f"ODBC:{dsn}|layername=points|sql={sql}"

# Create layer
layer = QgsVectorLayer(uri, "Leaf Points 2024", "ogr")

# Verify and add
if layer.isValid():
    # Configure CRS
    layer.setCrs(QgsCoordinateReferenceSystem("EPSG:4326"))
    
    # Add to project
    QgsProject.instance().addMapLayer(layer)
    
    # Zoom to extent
    iface.mapCanvas().setExtent(layer.extent())
    iface.mapCanvas().refresh()
    
    print(f"Layer added: {layer.featureCount()} features")
else:
    print("Error:", layer.error().message())
```

## Important Notes

1. **Read-only**: The driver is read-only. It's not possible to edit data via QGIS. Only SELECT queries are supported.

2. **WKT Geometry**: The `geometry` column must contain geometries in WKT (Well-Known Text) format. Examples:
   - Point: `POINT(-47.123 -23.456)`
   - LineString: `LINESTRING(-47.1 -23.4, -47.2 -23.5)`
   - Polygon: `POLYGON((-47.1 -23.4, -47.2 -23.5, -47.1 -23.6, -47.1 -23.4))`

3. **Default SRID**: The driver assumes SRID 4326 (WGS84). If your data uses another CRS, configure manually in QGIS layer properties.

4. **Performance**: Always use `LIMIT` in queries to avoid loading millions of points at once. Large datasets can cause QGIS to hang.

5. **Connection String Format**: QGIS uses OGR which expects the format:
   - Simple: `ODBC:DSN_NAME`
   - With credentials: `ODBC:Username=user;Password=pass@DSN_NAME`
   - With SQL: `ODBC:DSN_NAME|layername=table|sql=SELECT ...`

6. **Driver Installation**: The driver must be installed as a system library (`.so` or `.dylib`), not as a ZIP file. QGIS/GDAL loads it dynamically at runtime.

7. **QGIS Version**: QGIS 3.x is recommended. Older versions may have limited ODBC support.

## Quick Reference

### Connection String Formats

**Simple DSN:**
```
ODBC:LeafPointLake
```

**With Credentials:**
```
ODBC:Username=your_user;Password=your_pass@LeafPointLake
```

**With SQL Query:**
```
ODBC:LeafPointLake|layername=points|sql=SELECT geometry, timestamp FROM leaf.pointlake.points LIMIT 100
```

### Common SQL Queries

**Load sample points:**
```sql
SELECT geometry, timestamp, operationType 
FROM leaf.pointlake.points 
LIMIT 1000
```

**Filter by date:**
```sql
SELECT geometry, timestamp 
FROM leaf.pointlake.points 
WHERE timestamp >= '2024-01-01'
LIMIT 5000
```

**Sample large dataset:**
```sql
SELECT * 
FROM leaf.pointlake.points 
TABLESAMPLE (0.1 PERCENT) 
LIMIT 10000
```
