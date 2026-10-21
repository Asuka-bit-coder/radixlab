#pragma once

#include <stdexcept>

namespace radixlab {
namespace detail {

[[nodiscard]]
constexpr bool isValidRadix(int radix) noexcept {
    return radix >= 2 && radix <= 36;
}

// Map an ASCII digit to its value. Case-insensitive for A-Z.
[[nodiscard]]
inline int digitValue(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }

    if (character >= 'A' && character <= 'Z') {
        return character - 'A' + 10;
    }

    if (character >= 'a' && character <= 'z') {
        return character - 'a' + 10;
    }

    throw std::invalid_argument("Invalid digit character");
}

// Map a value in [0, 35] to an uppercase digit character.
[[nodiscard]]
inline char digitCharacter(int digit) {
    if (digit >= 0 && digit <= 9) {
        return static_cast<char>('0' + digit);
    }

    if (digit >= 10 && digit <= 35) {
        return static_cast<char>('A' + digit - 10);
    }

    throw std::logic_error("Digit is outside the supported range");
}

} // namespace detail
} // namespace radixlab
