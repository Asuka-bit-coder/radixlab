#include "radixlab/parser.hpp"

#include "radixlab/digits.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace radixlab {

// Parse a numeral using Horner's method.
//
// value_next = value_current * radix + digit
std::int64_t RadixParser::parse(
    std::string_view input,
    int radix
) {

    // --------------------------------------------------
    // Step 1: Validate radix
    // --------------------------------------------------

    if (!detail::isValidRadix(radix)) {
        throw std::invalid_argument(
            "Radix must be between 2 and 36"
        );
    }


    // --------------------------------------------------
    // Step 2: Validate input
    // --------------------------------------------------

    if (input.empty()) {
        throw std::invalid_argument(
            "Input cannot be empty"
        );
    }


    // --------------------------------------------------
    // Step 3: Parse optional sign
    // --------------------------------------------------

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


    // --------------------------------------------------
    // Step 4: Configure numeric limits
    // --------------------------------------------------

    const std::uint64_t maxPositive =
        static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()
        );

    // INT64_MIN has an absolute magnitude one greater
    // than INT64_MAX.
    const std::uint64_t limit =
        negative
            ? maxPositive + std::uint64_t{1}
            : maxPositive;

    const std::uint64_t base =
        static_cast<std::uint64_t>(radix);

    std::uint64_t value = 0;


    // --------------------------------------------------
    // Step 5: Parse digits using Horner's method
    // --------------------------------------------------

    for (std::size_t i = pos; i < input.size(); ++i) {

        const int digit = detail::digitValue(input[i]);

        // A digit must be smaller than the radix.
        if (digit >= radix) {
            throw std::invalid_argument(
                "Digit out of range for radix"
            );
        }

        const std::uint64_t d =
            static_cast<std::uint64_t>(digit);

        // Prevent overflow BEFORE multiplication.
        //
        // value * base + d <= limit
        //
        // Therefore:
        //
        // value <= (limit - d) / base
        if (value > (limit - d) / base) {
            throw std::out_of_range(
                "Integer overflow"
            );
        }

        value = value * base + d;
    }


    // --------------------------------------------------
    // Step 6: Apply sign safely
    // --------------------------------------------------

    if (negative) {

        // Handle INT64_MIN separately.
        if (value == maxPositive + std::uint64_t{1}) {
            return std::numeric_limits<std::int64_t>::min();
        }

        return -static_cast<std::int64_t>(value);
    }

    return static_cast<std::int64_t>(value);
}

} // namespace radixlab
