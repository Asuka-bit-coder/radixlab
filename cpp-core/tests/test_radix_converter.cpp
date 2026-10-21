#include "radixlab/radix_converter.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>

using radixlab::ConversionResult;
using radixlab::RadixConverter;
using radixlab::Rational;

namespace {

int passed = 0;
int failed = 0;

void check(std::string_view name, bool condition) {
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << name << '\n';
    } else {
        ++failed;
        std::cerr << "[FAIL] " << name << '\n';
    }
}

void expectConversion(
    std::string_view name,
    std::string_view input,
    int sourceBase,
    int targetBase,
    std::string_view expected,
    bool expectedExact = true,
    bool expectedRepeating = false,
    bool expectedTruncated = false,
    std::size_t maxFractionDigits = 256
) {
    try {
        const ConversionResult actual = RadixConverter::convert(
            input,
            sourceBase,
            targetBase,
            maxFractionDigits
        );

        const bool matches =
            actual.result == expected &&
            actual.exact == expectedExact &&
            actual.repeating == expectedRepeating &&
            actual.truncated == expectedTruncated;

        if (!matches) {
            std::cerr
                << "  expected: " << expected
                << " exact=" << expectedExact
                << " repeating=" << expectedRepeating
                << " truncated=" << expectedTruncated << '\n'
                << "  actual:   " << actual.result
                << " exact=" << actual.exact
                << " repeating=" << actual.repeating
                << " truncated=" << actual.truncated << '\n';
        }

        check(name, matches);
    } catch (const std::exception& error) {
        std::cerr << "  unexpected exception: " << error.what() << '\n';
        check(name, false);
    }
}

void expectRationalConversion(
    std::string_view name,
    const Rational& value,
    int targetBase,
    std::string_view expected,
    bool expectedExact = true,
    bool expectedRepeating = false,
    std::size_t maxFractionDigits = 256
) {
    try {
        const ConversionResult actual = RadixConverter::convert(
            value,
            targetBase,
            maxFractionDigits
        );

        if (actual.result != expected ||
            actual.exact != expectedExact ||
            actual.repeating != expectedRepeating ||
            actual.truncated == expectedExact) {
            std::cerr << "  actual: " << actual.result
                      << " exact=" << actual.exact
                      << " repeating=" << actual.repeating
                      << " truncated=" << actual.truncated << '\n';
        }

        check(
            name,
            actual.result == expected &&
            actual.exact == expectedExact &&
            actual.repeating == expectedRepeating &&
            actual.truncated == !expectedExact
        );
    } catch (const std::exception& error) {
        std::cerr << "  unexpected exception: " << error.what() << '\n';
        check(name, false);
    }
}

void expectInvalid(
    std::string_view name,
    std::string_view input,
    int sourceBase,
    int targetBase
) {
    try {
        (void)RadixConverter::convert(input, sourceBase, targetBase);
        check(name, false);
    } catch (const std::invalid_argument&) {
        check(name, true);
    } catch (...) {
        check(name, false);
    }
}

} // namespace


int main() {
    std::cout << "=== Radix Converter Tests ===\n\n";

    // Documented examples.
    expectConversion(
        "Binary fraction to hexadecimal",
        "101101.011", 2, 16, "2D.6"
    );

    expectConversion(
        "One tenth to binary cycle",
        "0.1", 10, 2, "0.0(0011)", true, true
    );

    expectRationalConversion(
        "One third to decimal cycle",
        Rational{1, 3}, 10, "0.(3)", true, true
    );

    // Integer and sign coverage.
    expectConversion("Decimal to binary", "45", 10, 2, "101101");
    expectConversion("Binary to decimal", "101101", 2, 10, "45");
    expectConversion("Octal to decimal", "773", 8, 10, "507");
    expectConversion("Negative conversion", "-255", 10, 16, "-FF");
    expectConversion("Negative fraction", "-1.5", 10, 2, "-1.1");
    expectConversion("Zero normalization", "000.000", 10, 36, "0");
    expectConversion("Negative zero", "-0.0", 10, 2, "0");
    expectConversion("Case-insensitive input", "2af.8", 16, 10, "687.5");
    expectConversion("Explicit positive sign", "+z", 36, 10, "35");
    expectConversion(
        "Arbitrary precision above int64",
        "18446744073709551616", 10, 2,
        "10000000000000000000000000000000000000000000000000000000000000000"
    );
    expectConversion(
        "Negative arbitrary precision",
        "-18446744073709551616", 10, 16,
        "-10000000000000000"
    );

    // Every supported radix boundary.
    expectConversion("Base 2 minimum", "1", 2, 36, "1");
    expectConversion("Base 36 maximum", "10", 36, 10, "36");
    expectConversion("Base 36 uppercase output", "35", 10, 36, "Z");

    // Termination and repetition.
    expectConversion("Terminating fraction", "0.5", 10, 2, "0.1");
    expectConversion("Terminating octal", "0.25", 10, 8, "0.2");
    expectConversion("Alternative cycle notation", "1.0", 10, 3, "1");
    expectRationalConversion("One seventh", Rational{1, 7}, 10, "0.(142857)", true, true);
    expectRationalConversion(
        "Cycle after prefix",
        Rational{1, 6}, 10, "0.1(6)", true, true
    );
    expectRationalConversion(
        "Binary finite fraction",
        Rational{363, 8}, 16, "2D.6"
    );

    // Truncation must be explicit.
    expectConversion(
        "Truncated binary cycle",
        "0.1", 10, 2, "0.00", false, false, true, 2
    );
    expectConversion(
        "Zero fraction limit",
        "0.1", 10, 2, "0", false, false, true, 0
    );
    expectConversion(
        "Cycle fits exactly at limit",
        "0.1", 10, 2, "0.0(0011)", true, true, false, 5
    );

    // Upstream parser validation is preserved.
    expectInvalid("Invalid source base", "10", 1, 10);
    expectInvalid("Invalid target base", "10", 10, 37);
    expectInvalid("Invalid digit", "102", 2, 10);
    expectInvalid("Empty numeral", "", 10, 2);
    expectInvalid("Multiple radix points", "1.2.3", 10, 2);
    expectInvalid("Sign only", "-", 10, 2);

    std::cout << "\nPassed: " << passed
              << "\nFailed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
