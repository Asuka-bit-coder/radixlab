#include "radixlab/encoding/bit_vector.hpp"

#include "radixlab/digits.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace radixlab::encoding {

namespace {

BigInt powerOfTwo(std::size_t width) {
    return BigInt{1} << static_cast<unsigned int>(width);
}

void requireSameWidth(
    const BitVector& left,
    const BitVector& right,
    const char* operation
) {
    if (left.width() != right.width()) {
        throw std::invalid_argument(
            std::string(operation) + " requires equal bit widths"
        );
    }
}

} // namespace


BitVector::BitVector(BigInt pattern, std::size_t width)
    : pattern_(std::move(pattern)),
      width_(width) {
    validateWidth(width_);

    if (pattern_ < 0) {
        throw std::invalid_argument("Bit pattern cannot be negative");
    }

    if (pattern_ >= powerOfTwo(width_)) {
        throw std::invalid_argument("Bit pattern does not fit the given width");
    }
}


void BitVector::validateWidth(std::size_t width) {
    if (width < 1 || width > kMaxWidth) {
        throw std::invalid_argument("Bit width must be between 1 and 64");
    }
}


BigInt BitVector::maskForWidth(std::size_t width) {
    return powerOfTwo(width) - 1;
}


BitVector BitVector::zero(std::size_t width) {
    return BitVector{BigInt{0}, width};
}


BitVector BitVector::fromBinary(std::string_view binary) {
    if (binary.empty()) {
        throw std::invalid_argument("Binary bit string cannot be empty");
    }

    if (binary.size() > kMaxWidth) {
        throw std::invalid_argument("Binary bit string exceeds 64 bits");
    }

    BigInt pattern = 0;
    for (const char digit : binary) {
        if (digit != '0' && digit != '1') {
            throw std::invalid_argument(
                "Binary bit string may contain only '0' and '1'"
            );
        }

        pattern <<= 1U;
        if (digit == '1') {
            pattern += 1;
        }
    }

    return BitVector{std::move(pattern), binary.size()};
}


BitVector BitVector::fromHex(std::string_view hex, std::size_t width) {
    validateWidth(width);

    if (hex.empty()) {
        throw std::invalid_argument("Hexadecimal bit pattern cannot be empty");
    }

    const std::size_t maxDigits = (width + 3U) / 4U;
    if (hex.size() > maxDigits) {
        throw std::invalid_argument(
            "Hexadecimal pattern has too many digits for the width"
        );
    }

    BigInt pattern = 0;
    for (const char character : hex) {
        const int digit = detail::digitValue(character);
        if (digit < 0 || digit > 15) {
            throw std::invalid_argument(
                "Hexadecimal pattern may contain only 0-9, A-F"
            );
        }

        pattern <<= 4U;
        pattern += digit;
    }

    return BitVector{std::move(pattern), width};
}


std::size_t BitVector::width() const noexcept {
    return width_;
}


const BigInt& BitVector::pattern() const noexcept {
    return pattern_;
}


BigInt BitVector::toUnsigned() const {
    return pattern_;
}


bool BitVector::bit(std::size_t index) const {
    if (index >= width_) {
        throw std::out_of_range("Bit index is outside the vector width");
    }

    return ((pattern_ >> static_cast<unsigned int>(index)) & 1) != 0;
}


bool BitVector::signBit() const {
    return bit(width_ - 1U);
}


std::string BitVector::toBinary() const {
    std::string result;
    result.reserve(width_);

    for (std::size_t index = width_; index-- > 0;) {
        result.push_back(bit(index) ? '1' : '0');
    }

    return result;
}


std::string BitVector::toHex() const {
    const std::size_t digitCount = (width_ + 3U) / 4U;
    std::string result(digitCount, '0');
    BigInt remaining = pattern_;

    for (std::size_t index = digitCount; index-- > 0;) {
        const int digit = static_cast<int>(
            (remaining & 0xFU).convert_to<unsigned int>()
        );
        result[index] = detail::digitCharacter(digit);
        remaining >>= 4U;
    }

    return result;
}


BitVector BitVector::shiftLeft(std::size_t amount) const {
    if (amount >= width_) {
        return zero(width_);
    }

    const BigInt shifted =
        (pattern_ << static_cast<unsigned int>(amount)) & maskForWidth(width_);
    return BitVector{shifted, width_};
}


BitVector BitVector::shiftRight(std::size_t amount) const {
    if (amount >= width_) {
        return zero(width_);
    }

    const BigInt shifted =
        pattern_ >> static_cast<unsigned int>(amount);
    return BitVector{shifted, width_};
}


BitVector BitVector::bitwiseAnd(const BitVector& other) const {
    requireSameWidth(*this, other, "bitwise AND");
    return BitVector{pattern_ & other.pattern_, width_};
}


BitVector BitVector::bitwiseOr(const BitVector& other) const {
    requireSameWidth(*this, other, "bitwise OR");
    return BitVector{pattern_ | other.pattern_, width_};
}


BitVector BitVector::bitwiseXor(const BitVector& other) const {
    requireSameWidth(*this, other, "bitwise XOR");
    return BitVector{pattern_ ^ other.pattern_, width_};
}


BitVector BitVector::bitwiseNot() const {
    return BitVector{maskForWidth(width_) ^ pattern_, width_};
}


BitVector BitVector::withBit(std::size_t index, bool value) const {
    if (index >= width_) {
        throw std::out_of_range("Bit index is outside the vector width");
    }

    const BigInt mask = BigInt{1} << static_cast<unsigned int>(index);
    BigInt pattern = pattern_;

    if (value) {
        pattern |= mask;
    } else {
        pattern &= (maskForWidth(width_) ^ mask);
    }

    return BitVector{std::move(pattern), width_};
}


BitVector BitVector::flipBit(std::size_t index) const {
    if (index >= width_) {
        throw std::out_of_range("Bit index is outside the vector width");
    }

    const BigInt mask = BigInt{1} << static_cast<unsigned int>(index);
    return BitVector{pattern_ ^ mask, width_};
}


std::size_t countSetBits(const BitVector& bits) noexcept {
    std::size_t count = 0;
    for (std::size_t index = 0; index < bits.width(); ++index) {
        if (((bits.pattern() >> static_cast<unsigned int>(index)) & 1) != 0) {
            ++count;
        }
    }
    return count;
}

} // namespace radixlab::encoding
