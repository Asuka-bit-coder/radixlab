cmake_minimum_required(VERSION 3.20)

project(
    radixlab
    VERSION 0.1.0
    DESCRIPTION "Exact radix conversion engine and CLI"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use Boost's package configuration directly. This is compatible with CMake 4,
# where the legacy FindBoost module was removed.
find_package(Boost 1.70 REQUIRED CONFIG)

# ----------------------------------------
# Core library
# ----------------------------------------

add_library(radixlab_core STATIC
    src/parser.cpp
    src/rational.cpp
    src/rational_parser.cpp
    src/radix_converter.cpp
)

target_include_directories(radixlab_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(radixlab_core PUBLIC
    Boost::boost
)

# ----------------------------------------
# CLI
# ----------------------------------------

add_executable(radixlab-cli
    cli/main.cpp
)

target_link_libraries(radixlab-cli PRIVATE
    radixlab_core
)

# ----------------------------------------
# Compiler warnings
# ----------------------------------------

function(radixlab_enable_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
        )
    endif()
endfunction()

radixlab_enable_warnings(radixlab_core)
radixlab_enable_warnings(radixlab-cli)

# ----------------------------------------
# Testing
# ----------------------------------------

include(CTest)

if(BUILD_TESTING)
    add_executable(test_parser tests/test_parser.cpp)
    target_link_libraries(test_parser PRIVATE radixlab_core)
    radixlab_enable_warnings(test_parser)

    add_executable(test_rational tests/test_rational.cpp)
    target_link_libraries(test_rational PRIVATE radixlab_core)
    radixlab_enable_warnings(test_rational)

    add_executable(test_rational_parser tests/test_rational_parser.cpp)
    target_link_libraries(test_rational_parser PRIVATE radixlab_core)
    radixlab_enable_warnings(test_rational_parser)

    add_executable(test_radix_converter tests/test_radix_converter.cpp)
    target_link_libraries(test_radix_converter PRIVATE radixlab_core)
    radixlab_enable_warnings(test_radix_converter)

    add_test(NAME RadixParserTests COMMAND test_parser)
    add_test(NAME RationalTests COMMAND test_rational)
    add_test(NAME RationalParserTests COMMAND test_rational_parser)
    add_test(NAME RadixConverterTests COMMAND test_radix_converter)

    add_test(
        NAME CliTests
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:radixlab-cli>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.cmake
    )
endif()

# ----------------------------------------
# Installation
# ----------------------------------------

include(GNUInstallDirs)

install(
    TARGETS radixlab_core radixlab-cli
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
