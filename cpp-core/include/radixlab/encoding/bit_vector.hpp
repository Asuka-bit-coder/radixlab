#pragma once

#include "radixlab/big_int.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace radixlab::encoding {

/**
 * A fixed-width bit pattern.
 *
 * BitVector stores an unsigned bit pattern, not a signed mathematical value.
 * Interpretation is supplied separately by an encoding such as sign-magnitude
 * or two's complement. Bit index 0 is the least-significant bit.
 *
 * Widths from 1 through 64 are accepted. All arithmetic uses cpp_int, so
 * shifts and masks never rely on signed overflow or implementation-defined
 * 64-bit behavior.
 */
class BitVector {
public:
    static constexpr std::size_t kMaxWidth = 64;

    BitVector(BigInt pattern, std::size_t width);

    [[nodiscard]]
    static BitVector zero(std::size_t width);

    /**
     * Parse exactly one binary digit per bit, MSB first. Whitespace and
     * prefixes are rejected.
     */
    [[nodiscard]]
    static BitVector fromBinary(std::string_view binary);

    /**
     * Parse up to ceil(width / 4) hexadecimal digits, MSB first.
     * The value must fit within width bits.
     */
    [[nodiscard]]
    static BitVector fromHex(std::string_view hex, std::size_t width);

    [[nodiscard]]
    std::size_t width() const noexcept;

    [[nodiscard]]
    const BigInt& pattern() const noexcept;

    [[nodiscard]]
    BigInt toUnsigned() const;

    [[nodiscard]]
    bool bit(std::size_t index) const;

    [[nodiscard]]
    bool signBit() const;

    [[nodiscard]]
    std::string toBinary() const;

    /**
     * Uppercase hexadecimal, zero-extended to a whole number of nibbles.
     */
    [[nodiscard]]
    std::string toHex() const;

    // Logical shifts: vacated bits are zero. Shifting by at least the width
    // yields an all-zero vector; bits shifted beyond the width are discarded.
    [[nodiscard]]
    BitVector shiftLeft(std::size_t amount) const;
    [[nodiscard]]
    BitVector shiftRight(std::size_t amount) const;

    // Widths must match for binary bit operations.
    [[nodiscard]]
    BitVector bitwiseAnd(const BitVector& other) const;
    [[nodiscard]]
    BitVector bitwiseOr(const BitVector& other) const;
    [[nodiscard]]
    BitVector bitwiseXor(const BitVector& other) const;
    [[nodiscard]]
    BitVector bitwiseNot() const;

    [[nodiscard]]
    BitVector withBit(std::size_t index, bool value) const;
    [[nodiscard]]
    BitVector flipBit(std::size_t index) const;

    bool operator==(const BitVector&) const = default;

private:
    BigInt pattern_;
    std::size_t width_;

    static void validateWidth(std::size_t width);
    static BigInt maskForWidth(std::size_t width);
};

[[nodiscard]]
std::size_t countSetBits(const BitVector& bits) noexcept;

} // namespace radixlab::encoding
