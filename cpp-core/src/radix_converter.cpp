#include "radixlab/radix_converter.hpp"

#include "radixlab/digits.hpp"
#include "radixlab/rational_parser.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

namespace radixlab {

namespace {

std::string integerToDigits(BigInt value, int base) {
    if (value == 0) {
        return "0";
    }

    std::string digits;
    while (value > 0) {
        const int digit = static_cast<int>(
            (value % base).convert_to<unsigned int>()
        );
        digits.push_back(detail::digitCharacter(digit));
        value /= base;
    }

    std::reverse(digits.begin(), digits.end());
    return digits;
}

void validateBases(int sourceBase, int targetBase) {
    if (!detail::isValidRadix(sourceBase)) {
        throw std::invalid_argument("Source base must be between 2 and 36");
    }

    if (!detail::isValidRadix(targetBase)) {
        throw std::invalid_argument("Target base must be between 2 and 36");
    }
}

} // namespace


ConversionResult RadixConverter::convert(
    std::string_view input,
    int sourceBase,
    int targetBase,
    std::size_t maxFractionDigits
) {
    validateBases(sourceBase, targetBase);
    const Rational value = RationalParser::parse(input, sourceBase);
    return convert(value, targetBase, maxFractionDigits);
}


ConversionResult RadixConverter::convert(
    const Rational& value,
    int targetBase,
    std::size_t maxFractionDigits
) {
    if (!detail::isValidRadix(targetBase)) {
        throw std::invalid_argument("Target base must be between 2 and 36");
    }

    const bool negative = value.numerator() < 0;
    const BigInt magnitude = negative
        ? -value.numerator()
        : value.numerator();

    const BigInt base = targetBase;
    const BigInt integerPart = magnitude / value.denominator();
    BigInt remainder = magnitude % value.denominator();

    std::string fraction;
    fraction.reserve(std::min<std::size_t>(maxFractionDigits, 128));

    // Map each remainder to the index at which it was first observed.
    // std::map avoids relying on a hash specialization for cpp_int.
    std::map<BigInt, std::size_t> seenRemainders;
    std::size_t cycleStart = 0;
    bool repeating = false;
    bool truncated = false;

    while (remainder != 0) {
        const auto found = seenRemainders.find(remainder);
        if (found != seenRemainders.end()) {
            cycleStart = found->second;
            repeating = true;
            break;
        }

        if (fraction.size() >= maxFractionDigits) {
            truncated = true;
            break;
        }

        seenRemainders.emplace(remainder, fraction.size());

        const BigInt scaled = remainder * base;
        const int digit = static_cast<int>(
            (scaled / value.denominator()).convert_to<unsigned int>()
        );
        fraction.push_back(detail::digitCharacter(digit));
        remainder = scaled % value.denominator();
    }

    std::string result;
    if (negative && (integerPart != 0 || remainder != 0)) {
        result.push_back('-');
    }

    result += integerToDigits(integerPart, targetBase);

    if (!fraction.empty()) {
        result.push_back('.');
        if (repeating) {
            result.append(fraction, 0, cycleStart);
            result.push_back('(');
            result.append(fraction, cycleStart, std::string::npos);
            result.push_back(')');
        } else {
            result += fraction;
        }
    }

    return ConversionResult{
        .result = std::move(result),
        .exact = !truncated,
        .repeating = repeating,
        .truncated = truncated
    };
}

} // namespace radixlab
