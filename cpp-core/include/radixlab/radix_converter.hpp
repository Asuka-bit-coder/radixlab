#pragma once

#include "radixlab/rational.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace radixlab {

/**
 * Result of converting an exact rational value to a positional numeral.
 *
 * exact      : the complete value is represented (terminating or repeating).
 * repeating  : the fractional part has a detected repeating cycle.
 * truncated  : maxFractionDigits was reached before the value was represented.
 */
struct ConversionResult {
    std::string result;
    bool exact{false};
    bool repeating{false};
    bool truncated{false};
};

class RadixConverter {
public:
    /**
     * Parse a positional numeral and convert it to another radix.
     *
     * @param input Numeral in sourceBase, with an optional sign and at most
     *              one radix point.
     * @param sourceBase Base between 2 and 36.
     * @param targetBase Base between 2 and 36.
     * @param maxFractionDigits Maximum number of fractional digits to emit.
     *
     * @throws std::invalid_argument For invalid bases, numerals, or limits.
     * @throws std::out_of_range If conversion would exceed internal limits.
     */
    [[nodiscard]]
    static ConversionResult convert(
        std::string_view input,
        int sourceBase,
        int targetBase,
        std::size_t maxFractionDigits = 256
    );

    /**
     * Convert an exact rational to a positional numeral in targetBase.
     *
     * Exit digits are uppercase. A complete cycle is written as
     * "prefix(cycle)". Truncated output contains no ellipsis; callers must
     * inspect truncated.
     */
    [[nodiscard]]
    static ConversionResult convert(
        const Rational& value,
        int targetBase,
        std::size_t maxFractionDigits = 256
    );
};

} // namespace radixlab
