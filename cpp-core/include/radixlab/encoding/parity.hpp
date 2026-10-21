#pragma once

#include "radixlab/encoding/bit_vector.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace radixlab::encoding {

enum class ParityMode {
    Even,
    Odd
};

[[nodiscard]]
ParityMode parseParityMode(std::string_view name);

[[nodiscard]]
std::string_view parityModeName(ParityMode mode) noexcept;

struct ParityResult {
    std::size_t onesCount;
    bool parityBit;
    // Data bits followed by the generated parity bit at the right.
    std::string dataWithParity;
};

/**
 * Generate the parity bit that gives the data-plus-parity word the requested
 * even or odd number of one bits.
 */
[[nodiscard]]
ParityResult generateParity(const BitVector& data, ParityMode mode);

/**
 * Verify a received data word against a received parity bit.
 *
 * A false result means the received parity bit differs from the parity
 * recomputed from the received data, which detects an odd number of changed
 * bits across the data-plus-parity word. It does not locate or correct the
 * error, and an even number of changes can go undetected.
 */
[[nodiscard]]
bool verifyParity(
    const BitVector& receivedData,
    bool receivedParityBit,
    ParityMode mode
);

} // namespace radixlab::encoding
