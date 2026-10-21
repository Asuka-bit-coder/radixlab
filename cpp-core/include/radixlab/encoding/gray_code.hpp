#pragma once

#include "radixlab/big_int.hpp"
#include "radixlab/encoding/bit_vector.hpp"

#include <cstddef>

namespace radixlab::encoding {

struct GrayResult {
    std::size_t width;
    BigInt value;
    BitVector binary;
    BitVector gray;
};

/**
 * Binary to reflected Gray code: G = B xor (B >> 1).
 */
[[nodiscard]]
BitVector binaryToGray(const BitVector& binary);

/**
 * Reflected Gray code to binary. The most-significant binary bit equals the
 * most-significant Gray bit; each lower binary bit is the XOR of the binary
 * bit above it and the corresponding Gray bit.
 */
[[nodiscard]]
BitVector grayToBinary(const BitVector& gray);

/**
 * Encode unsigned value in the requested width. Throws RangeError when value
 * is outside [0, 2^width - 1].
 */
[[nodiscard]]
GrayResult encodeGray(BigInt value, std::size_t width);

/**
 * Decode a Gray pattern and report both binary and mathematical value.
 */
[[nodiscard]]
GrayResult decodeGray(const BitVector& gray);

} // namespace radixlab::encoding
