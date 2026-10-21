#pragma once

#include "radixlab/big_int.hpp"
#include "radixlab/encoding/bit_vector.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace radixlab::encoding {

enum class SignedEncoding {
    SignMagnitude,
    OnesComplement,
    TwosComplement
};

/**
 * Parse SIGN_MAGNITUDE, ONES_COMPLEMENT, or TWOS_COMPLEMENT. Hyphens,
 * underscores, spaces, and letter case are accepted.
 */
[[nodiscard]]
SignedEncoding parseSignedEncoding(std::string_view name);

[[nodiscard]]
std::string_view signedEncodingName(SignedEncoding encoding) noexcept;

struct SignedRange {
    BigInt minimum;
    BigInt maximum;
};

[[nodiscard]]
SignedRange signedRange(std::size_t width, SignedEncoding encoding);

struct SignedEncodeResult {
    BigInt value;
    std::size_t width;
    SignedEncoding encoding;
    BitVector bits;
    bool negativeZero;
    SignedRange range;
};

struct SignedDecodeResult {
    BigInt value;
    std::size_t width;
    SignedEncoding encoding;
    BitVector bits;
    bool negativeZero;
    SignedRange range;
};

/**
 * Encode an integer. Sign-magnitude and ones' complement each have two zero
 * patterns; set negativeZero to request the negative-zero pattern for value 0.
 * Two's complement rejects negativeZero because zero is unique.
 *
 * Throws RangeError when value is outside the encoding's representable range.
 */
[[nodiscard]]
SignedEncodeResult encodeSigned(
    BigInt value,
    std::size_t width,
    SignedEncoding encoding,
    bool negativeZero = false
);

/**
 * Decode a bit pattern. For sign-magnitude or ones' complement, a zero value
 * decoded from the negative-zero pattern is reported with negativeZero=true.
 */
[[nodiscard]]
SignedDecodeResult decodeSigned(
    const BitVector& bits,
    SignedEncoding encoding
);

} // namespace radixlab::encoding
