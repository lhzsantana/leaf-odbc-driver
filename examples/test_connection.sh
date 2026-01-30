#!/bin/bash
# Basic test script for LeafODBC driver

set -e

echo "=== LeafODBC Connection Test ==="
echo ""

# Check if driver is registered
echo "1. Checking driver registration..."
if odbcinst -q -d | grep -q "LeafODBC"; then
    echo "   ✓ LeafODBC driver found"
else
    echo "   ✗ LeafODBC driver not found!"
    echo "   Run: odbcinst -i -d -f odbcinst.ini"
    exit 1
fi

# Check if DSN is configured
echo ""
echo "2. Checking DSN configuration..."
if odbcinst -q -s | grep -q "LeafPointLake"; then
    echo "   ✓ LeafPointLake DSN found"
else
    echo "   ✗ LeafPointLake DSN not found!"
    echo "   Configure DSN in ~/.odbc.ini or /etc/odbc.ini"
    exit 1
fi

# Test connection with isql
echo ""
echo "3. Testing connection..."
echo "   Executing: SELECT 1;"
echo ""

if echo "SELECT 1;" | isql -v LeafPointLake -b 2>&1 | grep -q "1"; then
    echo "   ✓ Connection successful!"
else
    echo "   ✗ Connection failed"
    echo "   Check credentials in odbc.ini"
    exit 1
fi

# Test GEOMETRY_COLUMNS
echo ""
echo "4. Testing GEOMETRY_COLUMNS table..."
if echo "SELECT * FROM GEOMETRY_COLUMNS;" | isql -v LeafPointLake -b 2>&1 | grep -q "points"; then
    echo "   ✓ GEOMETRY_COLUMNS accessible"
else
    echo "   ⚠ GEOMETRY_COLUMNS did not return expected data"
fi

# Test ogrinfo if available
if command -v ogrinfo &> /dev/null; then
    echo ""
    echo "5. Testing with ogrinfo (GDAL)..."
    if ogrinfo "ODBC:LeafPointLake" -al -q 2>&1 | grep -q "points"; then
        echo "   ✓ ogrinfo can access 'points' layer"
    else
        echo "   ⚠ ogrinfo could not access layer"
    fi
else
    echo ""
    echo "5. ogrinfo not found (optional)"
fi

echo ""
echo "=== Tests completed ==="
