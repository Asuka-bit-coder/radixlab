#include "radixlab/parser.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

using radixlab::RadixParser;

namespace {

int testsPassed = 0;
int testsFailed = 0;

void check(
    std::string_view name,
    bool condition
) {
    if (condition) {
        ++testsPassed;
        std::cout << "[PASS] " << name << '\n';
    } else {
        ++testsFailed;
        std::cerr << "[FAIL] " << name << '\n';
    }
}

void testValue(
    std::string_view name,
    std::string_view input,
    int radix,
    std::int64_t expected
) {
    try {
        const auto actual =
            RadixParser::parse(input, radix);

        check(name, actual == expected);
    }
    catch (const std::exception& e) {
        std::cerr << "Unexpected exception: "
                  << e.what() << '\n';

        check(name, false);
    }
}

template <typename ExceptionType>
void testException(
    std::string_view name,
    std::string_view input,
    int radix
) {
    try {
        (void)RadixParser::parse(input, radix);
        check(name, false);
    }
    catch (const ExceptionType&) {
        check(name, true);
    }
    catch (const std::exception& e) {
        std::cerr << "Wrong exception type: "
                  << e.what() << '\n';

        check(name, false);
    }
}

} // namespace

int main() {

    std::cout << "=== RadixLab Parser Tests ===\n\n";

    // ------------------------------------------
    // Basic conversions
    // ------------------------------------------

    testValue(
        "Binary to decimal",
        "101101", 2, 45
    );

    testValue(
        "Hexadecimal to decimal",
        "2AF", 16, 687
    );

    testValue(
        "Octal to decimal",
        "73", 8, 59
    );

    testValue(
        "Decimal parsing",
        "304", 10, 304
    );

    testValue(
        "Base 36",
        "z", 36, 35
    );


    // ------------------------------------------
    // Signs
    // ------------------------------------------

    testValue(
        "Negative hexadecimal",
        "-FF", 16, -255
    );

    testValue(
        "Explicit positive sign",
        "+1010", 2, 10
    );

    testValue(
        "Negative zero",
        "-0", 10, 0
    );


    // ------------------------------------------
    // Leading zeros
    // ------------------------------------------

    testValue(
        "Leading zeros",
        "0001101", 2, 13
    );


    // ------------------------------------------
    // Case handling
    // ------------------------------------------

    testValue(
        "Lowercase hexadecimal",
        "ff", 16, 255
    );

    testValue(
        "Uppercase hexadecimal",
        "FF", 16, 255
    );


    // ------------------------------------------
    // Numeric boundaries
    // ------------------------------------------

    testValue(
        "INT64_MAX",
        "9223372036854775807",
        10,
        std::numeric_limits<std::int64_t>::max()
    );

    testValue(
        "INT64_MIN",
        "-9223372036854775808",
        10,
        std::numeric_limits<std::int64_t>::min()
    );


    // ------------------------------------------
    // Invalid inputs
    // ------------------------------------------

    testException<std::invalid_argument>(
        "Invalid binary digit",
        "102", 2
    );

    testException<std::invalid_argument>(
        "Invalid hexadecimal digit",
        "1G", 16
    );

    testException<std::invalid_argument>(
        "Empty input",
        "", 10
    );

    testException<std::invalid_argument>(
        "Sign only",
        "-", 10
    );

    testException<std::invalid_argument>(
        "Invalid radix below range",
        "101", 1
    );

    testException<std::invalid_argument>(
        "Invalid radix above range",
        "101", 37
    );

    testException<std::invalid_argument>(
        "Invalid character",
        "10@1", 2
    );

    testException<std::invalid_argument>(
        "Whitespace rejected",
        " 123", 10
    );

    testException<std::invalid_argument>(
        "Prefix rejected",
        "0xFF", 16
    );


    // ------------------------------------------
    // Overflow tests
    // ------------------------------------------

    testException<std::out_of_range>(
        "Positive overflow",
        "9223372036854775808",
        10
    );

    testException<std::out_of_range>(
        "Negative overflow",
        "-9223372036854775809",
        10
    );

    testException<std::out_of_range>(
        "Large hexadecimal overflow",
        "FFFFFFFFFFFFFFFF",
        16
    );


    // ------------------------------------------
    // Summary
    // ------------------------------------------

    std::cout << "\n============================\n";

    std::cout << "Passed: " << testsPassed << '\n';
    std::cout << "Failed: " << testsFailed << '\n';

    std::cout << "============================\n";

    return testsFailed == 0 ? 0 : 1;
}
