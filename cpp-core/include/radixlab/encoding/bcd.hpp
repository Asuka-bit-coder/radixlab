#pragma once

#include "radixlab/big_int.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace radixlab::encoding {

/**
 * Result of unsigned 8421 BCD encoding or decoding.
 *
 * digits preserves leading zeros from the input digit string or BCD groups.
 * value is the canonical mathematical value. binary is the ordinary binary
 * representation of value, provided to contrast packed BCD with positional
 * binary.
 */
struct BcdResult {
    std::string digits;
    std::string bits;
    std::vector<std::string> groups;
    BigInt value;
    std::string binary;
};

/**
 * Encode a non-negative decimal digit string. Every character must be 0-9.
 * Leading zeros are preserved. There is no sign encoding in this protocol.
 */
[[nodiscard]]
BcdResult encodeBcd(std::string_view decimal);

/**
 * Decode packed 8421 BCD. ASCII spaces are accepted for readability; after
 * removing them, the bit string must be a non-empty multiple of four and every
 * nibble must be 0000-1001.
 *
 * Leading zero groups are retained in digits and groups.
 */
[[nodiscard]]
BcdResult decodeBcd(std::string_view bits);

} // namespace radixlab::encoding
