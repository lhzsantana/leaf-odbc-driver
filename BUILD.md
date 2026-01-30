# Build Guide

## Prerequisites

### macOS

```bash
brew install unixodbc curl cmake
```

### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install unixodbc-dev libcurl4-openssl-dev cmake build-essential
```

### Linux (RHEL/CentOS)

```bash
sudo yum install unixODBC-devel libcurl-devel cmake gcc-c++
```

## Compilation

```bash
# Create build directory
mkdir build
cd build

# Configure CMake
cmake ..

# Build
make

# (Optional) Install
sudo make install
```

## Output

The driver will be compiled as:
- **macOS**: `build/lib/libleafodbc.dylib`
- **Linux**: `build/lib/libleafodbc.so`

## Verification

After building, verify the file was created:

```bash
ls -lh build/lib/libleafodbc.*
```

## Troubleshooting

### Error: "Could not find unixODBC"

Install unixODBC:
- macOS: `brew install unixodbc`
- Ubuntu: `sudo apt-get install unixodbc-dev`

### Error: "Could not find libcurl"

Install libcurl:
- macOS: `brew install curl`
- Ubuntu: `sudo apt-get install libcurl4-openssl-dev`

### Error: "CMake version too old"

Update CMake:
- macOS: `brew upgrade cmake`
- Ubuntu: `sudo apt-get install cmake` or download from https://cmake.org

### C++17 Compilation Error

Make sure your compiler supports C++17:
- Clang (macOS): usually already supports it
- GCC: version 7 or higher

Check:
```bash
g++ --version
# or
clang++ --version
```

## Debug Build

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## Release Build

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## Clean Build

```bash
cd build
make clean
# or
rm -rf build
```
