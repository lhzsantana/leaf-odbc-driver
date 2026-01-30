cmake_minimum_required(VERSION 3.15)
project(LeafODBC VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTS "Build test suite" OFF)

# Output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(UNIXODBC REQUIRED odbc)
pkg_check_modules(CURL REQUIRED libcurl)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${UNIXODBC_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/odbc_api.cpp
    src/handles.cpp
    src/conn_string.cpp
    src/leaf_client.cpp
    src/resultset.cpp
    src/metadata.cpp
    src/sql_guard.cpp
)

# Header files
set(HEADERS
    include/leafodbc/handles.h
    include/leafodbc/conn_string.h
    include/leafodbc/leaf_client.h
    include/leafodbc/resultset.h
    include/leafodbc/metadata.h
    include/leafodbc/sql_guard.h
    include/leafodbc/common.h
)

# Download nlohmann/json if not present
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Shared library
add_library(leafodbc SHARED ${SOURCES} ${HEADERS})
target_link_libraries(leafodbc
    PRIVATE
    ${UNIXODBC_LIBRARIES}
    ${CURL_LIBRARIES}
)

target_include_directories(leafodbc
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Platform-specific settings
if(APPLE)
    set_target_properties(leafodbc PROPERTIES
        OUTPUT_NAME "leafodbc"
        SUFFIX ".dylib"
    )
elseif(UNIX)
    set_target_properties(leafodbc PROPERTIES
        OUTPUT_NAME "leafodbc"
        SUFFIX ".so"
    )
endif()

# Install rules
install(TARGETS leafodbc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/leafodbc
    DESTINATION include
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
