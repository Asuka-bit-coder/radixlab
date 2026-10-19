cmake_minimum_required(VERSION 3.20)

project(
    radixlab_core
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------------------
# Core library
# ----------------------------------------

add_library(radixlab_core STATIC
    src/parser.cpp
)

target_include_directories(radixlab_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ----------------------------------------
# Compiler warnings
# ----------------------------------------

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(radixlab_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
    )
endif()

# ----------------------------------------
# Testing
# ----------------------------------------

enable_testing()

add_executable(test_parser
    tests/test_parser.cpp
)

target_link_libraries(test_parser PRIVATE
    radixlab_core
)

add_test(
    NAME RadixParserTests
    COMMAND test_parser
)
