# QGIS Quick Start Guide

This is a quick reference guide for getting LeafODBC working with QGIS in 5 minutes.

## ⚡ Quick Installation (5 Steps)

### Step 1: Download Release
```bash
# Go to: https://github.com/lhzsantana/leaf-odbc-driver/releases
# Download: leafodbc-macos.tar.gz (macOS) or leafodbc-linux.tar.gz (Linux)
```

### Step 2: Extract and Install Driver
```bash
# Extract
tar -xzf leafodbc-macos.tar.gz
cd leafodbc-macos

# Install (macOS)
sudo cp libleafodbc.dylib /usr/local/lib/

# Install (Linux)
sudo cp libleafodbc.so /usr/local/lib/
```

**Important**: The driver must be installed as a **shared library** (`.so` or `.dylib`), not used directly from the ZIP/tar.gz. QGIS loads it dynamically at runtime.

### Step 3: Register Driver
Edit `/etc/odbcinst.ini` or `~/.odbcinst.ini`:

```ini
[LeafODBC]
Description=Leaf ODBC Driver
Driver=/usr/local/lib/libleafodbc.dylib  # or .so on Linux
```

Verify:
```bash
odbcinst -q -d | grep LeafODBC
```

### Step 4: Create DSN
Edit `~/.odbc.ini`:

```ini
[LeafPointLake]
Driver=LeafODBC
EndpointBase=https://api.withleaf.io
Username=YOUR_USERNAME
Password=YOUR_PASSWORD
SqlEngine=SPARK_SQL
```

Test:
```bash
isql -v LeafPointLake
```

### Step 5: Add Layer in QGIS

1. **Layer → Data Source Manager** (`Ctrl+L`)
2. **Vector** tab → **Database → ODBC**
3. Connection string: `ODBC:LeafPointLake`
4. Click **Add**

Done! 🎉

## Common Issues

### "Driver not found"
- Check driver path in `odbcinst.ini` is absolute and correct
- Verify file exists: `ls -l /usr/local/lib/libleafodbc.*`

### "Layer is not valid"
- Test with `isql -v LeafPointLake` first
- Check QGIS logs: **View → Panels → Log Messages**

### ODBC option not in QGIS
- Install GDAL: `brew install gdal` (macOS) or `sudo apt-get install gdal-bin` (Linux)
- Restart QGIS

## Full Documentation

- [Complete QGIS Setup Guide](QGIS_SETUP.md)
- [ODBC Configuration Guide](ODBC_SETUP.md)
