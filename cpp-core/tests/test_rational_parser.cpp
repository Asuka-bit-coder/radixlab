#include "radixlab/rational_parser.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

using radixlab::RationalParser;

namespace {

int passed = 0;
int failed = 0;

void check(
    std::string_view name,
    bool condition
) {
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << name << '\n';
    } else {
        ++failed;
        std::cerr << "[FAIL] " << name << '\n';
    }
}

void testValue(
    std::string_view name,
    std::string_view input,
    int radix,
    std::string_view expected
) {
    try {
        auto value = RationalParser::parse(input, radix);
        check(name, value.toString() == expected);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        check(name, false);
    }
}

void testInvalid(
    std::string_view name,
    std::string_view input,
    int radix
) {
    try {
        (void)RationalParser::parse(input, radix);
        check(name, false);
    }
    catch (const std::invalid_argument&) {
        check(name, true);
    }
    catch (...) {
        check(name, false);
    }
}

} // namespace


int main() {

    std::cout << "=== Rational Parser Tests ===\n\n";

    testValue(
        "Binary fraction",
        "101.11", 2, "23/4"
    );

    testValue(
        "Course example",
        "101101.011", 2, "363/8"
    );

    testValue(
        "Hexadecimal fraction",
        "2AF.8", 16, "1375/2"
    );

    testValue(
        "Decimal fraction",
        "0.1", 10, "1/10"
    );

    testValue(
        "Negative octal fraction",
        "-73.4", 8, "-119/2"
    );

    testValue(
        "Positive integer",
        "+42", 10, "42"
    );

    testValue(
        "Leading zeros",
        "0001.100", 2, "3/2"
    );

    testValue(
        "Fraction without integer digits",
        ".5", 10, "1/2"
    );

    testValue(
        "Trailing radix point",
        "5.", 10, "5"
    );

    testValue(
        "Arbitrary precision",
        "123456789012345678901234567890",
        10,
        "123456789012345678901234567890"
    );

    testInvalid(
        "Empty input",
        "", 10
    );

    testInvalid(
        "Invalid binary digit",
        "102.1", 2
    );

    testInvalid(
        "Multiple radix points",
        "1.2.3", 10
    );

    testInvalid(
        "Sign without digits",
        "-", 10
    );

    testInvalid(
        "Radix point only",
        ".", 10
    );

    testInvalid(
        "Invalid character",
        "1@2", 10
    );

    testInvalid(
        "Invalid radix",
        "101", 1
    );

    testInvalid(
        "Whitespace",
        " 1.5", 10
    );

    std::cout << "\nPassed: " << passed
              << "\nFailed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
