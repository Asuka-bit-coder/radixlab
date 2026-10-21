cmake_minimum_required(VERSION 3.20)

project(
    radixlab
    VERSION 0.2.0
    DESCRIPTION "Exact radix conversion and digital logic encoding engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer Boost's package configuration. CMake 4 removed the legacy FindBoost
# module, so fall back to locating the header-only include tree directly.
find_package(Boost 1.70 QUIET CONFIG)

if(NOT Boost_FOUND)
    find_path(
        Boost_INCLUDE_DIR
        NAMES boost/multiprecision/cpp_int.hpp
        DOC "Boost include directory"
    )

    if(NOT Boost_INCLUDE_DIR)
        message(FATAL_ERROR
            "Boost.Multiprecision headers were not found. "
            "Install Boost 1.70 or newer."
        )
    endif()

    add_library(Boost::boost INTERFACE IMPORTED)
    target_include_directories(Boost::boost
        INTERFACE "${Boost_INCLUDE_DIR}"
    )
    message(STATUS "Using header-only Boost from ${Boost_INCLUDE_DIR}")
endif()

# ----------------------------------------
# Core library
# ----------------------------------------

add_library(radixlab_core STATIC
    src/parser.cpp
    src/rational.cpp
    src/rational_parser.cpp
    src/radix_converter.cpp
    src/encoding/bit_vector.cpp
    src/encoding/signed_encoding.cpp
    src/encoding/bcd.cpp
    src/encoding/gray_code.cpp
    src/encoding/parity.cpp
    src/boolean/lexer.cpp
    src/boolean/ast.cpp
    src/boolean/parser.cpp
    src/boolean/evaluator.cpp
    src/boolean/truth_table.cpp
    src/boolean/canonical_forms.cpp
    src/boolean/equivalence.cpp
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
    cli/cli_io.cpp
    cli/encoding_commands.cpp
    cli/boolean_commands.cpp
)

target_link_libraries(radixlab-cli PRIVATE
    radixlab_core
)

target_compile_definitions(radixlab-cli PRIVATE
    RADIXLAB_VERSION="${PROJECT_VERSION}"
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

    add_executable(test_bit_vector tests/test_bit_vector.cpp)
    target_link_libraries(test_bit_vector PRIVATE radixlab_core)
    radixlab_enable_warnings(test_bit_vector)

    add_executable(test_signed_encoding tests/test_signed_encoding.cpp)
    target_link_libraries(test_signed_encoding PRIVATE radixlab_core)
    radixlab_enable_warnings(test_signed_encoding)

    add_executable(test_bcd tests/test_bcd.cpp)
    target_link_libraries(test_bcd PRIVATE radixlab_core)
    radixlab_enable_warnings(test_bcd)

    add_executable(test_gray_code tests/test_gray_code.cpp)
    target_link_libraries(test_gray_code PRIVATE radixlab_core)
    radixlab_enable_warnings(test_gray_code)

    add_executable(test_parity tests/test_parity.cpp)
    target_link_libraries(test_parity PRIVATE radixlab_core)
    radixlab_enable_warnings(test_parity)

    add_executable(test_boolean_lexer tests/test_boolean_lexer.cpp)
    target_link_libraries(test_boolean_lexer PRIVATE radixlab_core)
    radixlab_enable_warnings(test_boolean_lexer)

    add_executable(test_boolean_parser tests/test_boolean_parser.cpp)
    target_link_libraries(test_boolean_parser PRIVATE radixlab_core)
    radixlab_enable_warnings(test_boolean_parser)

    add_executable(test_boolean_evaluator tests/test_boolean_evaluator.cpp)
    target_link_libraries(test_boolean_evaluator PRIVATE radixlab_core)
    radixlab_enable_warnings(test_boolean_evaluator)

    add_executable(test_boolean_truth_table tests/test_boolean_truth_table.cpp)
    target_link_libraries(test_boolean_truth_table PRIVATE radixlab_core)
    radixlab_enable_warnings(test_boolean_truth_table)

    add_executable(test_boolean_equivalence tests/test_boolean_equivalence.cpp)
    target_link_libraries(test_boolean_equivalence PRIVATE radixlab_core)
    radixlab_enable_warnings(test_boolean_equivalence)

    add_test(NAME RadixParserTests COMMAND test_parser)
    add_test(NAME RationalTests COMMAND test_rational)
    add_test(NAME RationalParserTests COMMAND test_rational_parser)
    add_test(NAME RadixConverterTests COMMAND test_radix_converter)
    add_test(NAME BitVectorTests COMMAND test_bit_vector)
    add_test(NAME SignedEncodingTests COMMAND test_signed_encoding)
    add_test(NAME BcdTests COMMAND test_bcd)
    add_test(NAME GrayCodeTests COMMAND test_gray_code)
    add_test(NAME ParityTests COMMAND test_parity)
    add_test(NAME BooleanLexerTests COMMAND test_boolean_lexer)
    add_test(NAME BooleanParserTests COMMAND test_boolean_parser)
    add_test(NAME BooleanEvaluatorTests COMMAND test_boolean_evaluator)
    add_test(NAME BooleanTruthTableTests COMMAND test_boolean_truth_table)
    add_test(NAME BooleanEquivalenceTests COMMAND test_boolean_equivalence)

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
