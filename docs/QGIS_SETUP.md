# LeafODBC Configuration in QGIS

This document describes how to use the LeafODBC driver in QGIS to access Leaf PointLake data.

## Prerequisites

1. LeafODBC driver compiled and registered in unixODBC (see [ODBC_SETUP.md](ODBC_SETUP.md))
2. QGIS installed (version 3.x recommended)
3. GDAL/OGR with ODBC support

### Verify ODBC Support in GDAL

```bash
ogrinfo --formats | grep ODBC
```

If it doesn't appear, you may need to install/compile GDAL with ODBC support.

## Adding Layer in QGIS

### Method 1: Via Data Source Manager (OGR)

1. Open QGIS
2. Go to **Layer → Data Source Manager** (or press `Ctrl+L`)
3. Select the **Vector** tab
4. Under **Source Type**, choose **Database → ODBC**
5. Configure:
   - **Connection string**: `ODBC:LeafPointLake`
     - Or with credentials: `ODBC:Username=your_user;Password=your_pass@LeafPointLake`
   - **SQL**: (optional) SQL query to filter data
     - Example: `SELECT geometry, timestamp, operationType FROM leaf.pointlake.points LIMIT 1000`
6. Click **Add** and then **Close**

### Method 2: Via Browser Panel

1. In the **Browser** panel, expand **ODBC**
2. Expand your DSN (`LeafPointLake`)
3. Expand **leaf → pointlake**
4. Drag the **points** table to the map

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

- Verify DSN is configured correctly (`odbcinst -q -s`)
- Test connection with `isql -v LeafPointLake`
- Check QGIS logs: **View → Panels → Log Messages**

### Error: "No geometry column found"

- Verify the `GEOMETRY_COLUMNS` table is accessible:
  ```bash
  isql -v LeafPointLake -b
  SQL> SELECT * FROM GEOMETRY_COLUMNS;
  ```
- Manually configure the geometry column in layer properties

### Error: "Authentication failed"

- Verify credentials in `odbc.ini`
- Test with `isql` first
- Use connection string with explicit credentials in QGIS

### Slow Performance

- Use `LIMIT` in queries
- Use `TABLESAMPLE` to reduce volume
- Consider creating virtual views with appropriate filters
- Check if there are indexes in the API (depends on PointLake implementation)

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

1. **Read-only**: The driver is read-only. It's not possible to edit data via QGIS.
2. **WKT Geometry**: The `geometry` column must contain geometries in WKT (Well-Known Text) format.
3. **Default SRID**: The driver assumes SRID 4326. If your data uses another CRS, configure manually in QGIS.
4. **Limits**: Always use `LIMIT` to avoid loading millions of points at once.
