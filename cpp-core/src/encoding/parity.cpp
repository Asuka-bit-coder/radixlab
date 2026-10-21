#include "radixlab/encoding/parity.hpp"

#include "radixlab/big_int.hpp"

#include <cctype>
#include <stdexcept>
#include <string>

namespace radixlab::encoding {

namespace {

std::string normalizedName(std::string_view name) {
    std::string normalized;
    normalized.reserve(name.size());

    for (const char character : name) {
        const unsigned char byte = static_cast<unsigned char>(character);
        if (character == '-' || character == '_' || character == ' ') {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(byte)));
    }

    return normalized;
}

} // namespace


ParityMode parseParityMode(std::string_view name) {
    const std::string normalized = normalizedName(name);

    if (normalized == "even" || normalized == "evenparity" || normalized == "e") {
        return ParityMode::Even;
    }

    if (normalized == "odd" || normalized == "oddparity" || normalized == "o") {
        return ParityMode::Odd;
    }

    throw std::invalid_argument("Unknown parity mode; use EVEN or ODD");
}


std::string_view parityModeName(ParityMode mode) noexcept {
    switch (mode) {
        case ParityMode::Even:
            return "EVEN";
        case ParityMode::Odd:
            return "ODD";
    }

    return "UNKNOWN";
}


ParityResult generateParity(const BitVector& data, ParityMode mode) {
    const std::size_t ones = countSetBits(data);
    const bool evenNumberOfOnes = (ones % 2U) == 0U;

    const bool parityBit = mode == ParityMode::Even
        ? !evenNumberOfOnes
        : evenNumberOfOnes;

    std::string word = data.toBinary();
    word.push_back(parityBit ? '1' : '0');

    return ParityResult{
        .onesCount = ones,
        .parityBit = parityBit,
        .dataWithParity = std::move(word)
    };
}


bool verifyParity(
    const BitVector& receivedData,
    bool receivedParityBit,
    ParityMode mode
) {
    return generateParity(receivedData, mode).parityBit == receivedParityBit;
}

} // namespace radixlab::encoding
