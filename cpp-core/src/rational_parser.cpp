#include "radixlab/rational_parser.hpp"

#include "radixlab/digits.hpp"

#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace radixlab {

Rational RationalParser::parse(
    std::string_view input,
    int radix
) {

    // Step 1: Validate radix
    if (!detail::isValidRadix(radix)) {
        throw std::invalid_argument(
            "Radix must be between 2 and 36"
        );
    }

    // Step 2: Validate input
    if (input.empty()) {
        throw std::invalid_argument(
            "Input cannot be empty"
        );
    }

    // Step 3: Parse optional sign
    std::size_t pos = 0;
    bool negative = false;

    if (input[0] == '-') {
        negative = true;
        pos = 1;
    }
    else if (input[0] == '+') {
        pos = 1;
    }

    if (pos == input.size()) {
        throw std::invalid_argument(
            "Sign without digits"
        );
    }

    // Step 4: Parse digits
    BigInt numerator = 0;
    BigInt denominator = 1;

    const BigInt base = radix;

    bool decimalPointSeen = false;
    bool digitSeen = false;

    for (std::size_t i = pos; i < input.size(); ++i) {
        const char c = input[i];

        if (c == '.') {
            if (decimalPointSeen) {
                throw std::invalid_argument(
                    "Multiple radix points"
                );
            }

            decimalPointSeen = true;
            continue;
        }

        const int digit = detail::digitValue(c);

        if (digit >= radix) {
            throw std::invalid_argument(
                "Digit out of range for radix"
            );
        }

        digitSeen = true;

        // Horner's method
        numerator = numerator * base + digit;

        // Every digit after the radix point
        // increases the denominator by a factor of radix.
        if (decimalPointSeen) {
            denominator *= base;
        }
    }

    if (!digitSeen) {
        throw std::invalid_argument(
            "Input must contain at least one digit"
        );
    }

    // Step 5: Apply sign
    if (negative) {
        numerator = -numerator;
    }

    // Step 6: Construct normalized rational
    return Rational{
        numerator,
        denominator
    };
}

} // namespace radixlab
