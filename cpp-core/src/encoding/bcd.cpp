#include "radixlab/encoding/bcd.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace radixlab::encoding {

namespace {

std::string binaryRepresentation(BigInt value) {
    if (value == 0) {
        return "0";
    }

    std::string binary;
    while (value > 0) {
        binary.push_back((value & 1) != 0 ? '1' : '0');
        value >>= 1U;
    }

    std::reverse(binary.begin(), binary.end());
    return binary;
}


std::string removeSpaces(std::string_view bits) {
    std::string compact;
    compact.reserve(bits.size());

    for (const char character : bits) {
        if (character == ' ') {
            continue;
        }

        compact.push_back(character);
    }

    return compact;
}


std::string nibbleBits(int digit) {
    std::string nibble(4, '0');
    for (std::size_t bit = 0; bit < 4; ++bit) {
        const int shift = static_cast<int>(3U - bit);
        nibble[bit] = ((digit >> shift) & 1) != 0 ? '1' : '0';
    }
    return nibble;
}


int decodeNibble(std::string_view nibble) {
    int digit = 0;
    for (const char bit : nibble) {
        if (bit != '0' && bit != '1') {
            throw std::invalid_argument(
                "BCD bit string may contain only '0' and '1' (and spaces)"
            );
        }
        digit = (digit << 1) | (bit == '1' ? 1 : 0);
    }

    if (digit > 9) {
        throw std::invalid_argument(
            "Illegal BCD nibble: each group must be 0000 through 1001"
        );
    }

    return digit;
}

} // namespace


BcdResult encodeBcd(std::string_view decimal) {
    if (decimal.empty()) {
        throw std::invalid_argument("Decimal digit string cannot be empty");
    }

    BcdResult result;
    result.digits.reserve(decimal.size());
    result.bits.reserve(decimal.size() * 4U);
    result.groups.reserve(decimal.size());

    for (const char character : decimal) {
        if (character < '0' || character > '9') {
            throw std::invalid_argument(
                "BCD encoding accepts decimal digits 0-9 only"
            );
        }

        const int digit = character - '0';
        const std::string nibble = nibbleBits(digit);
        result.digits.push_back(character);
        result.groups.push_back(nibble);
        result.bits += nibble;
        result.value = result.value * 10 + digit;
    }

    result.binary = binaryRepresentation(result.value);
    return result;
}


BcdResult decodeBcd(std::string_view bits) {
    const std::string compact = removeSpaces(bits);

    if (compact.empty()) {
        throw std::invalid_argument("BCD bit string cannot be empty");
    }

    if (compact.size() % 4U != 0U) {
        throw std::invalid_argument(
            "BCD bit string length must be a multiple of four"
        );
    }

    BcdResult result;
    result.bits = compact;
    result.groups.reserve(compact.size() / 4U);
    result.digits.reserve(compact.size() / 4U);

    for (std::size_t offset = 0; offset < compact.size(); offset += 4U) {
        const std::string_view nibble(compact.data() + offset, 4U);
        const int digit = decodeNibble(nibble);

        result.groups.emplace_back(nibble);
        result.digits.push_back(static_cast<char>('0' + digit));
        result.value = result.value * 10 + digit;
    }

    result.binary = binaryRepresentation(result.value);
    return result;
}

} // namespace radixlab::encoding
