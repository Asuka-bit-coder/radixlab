#include "radixlab/encoding/gray_code.hpp"

#include "radixlab/encoding/encoding_error.hpp"

#include <stdexcept>
#include <utility>

namespace radixlab::encoding {

BitVector binaryToGray(const BitVector& binary) {
    const BigInt gray =
        binary.pattern() ^ (binary.pattern() >> 1U);
    return BitVector{gray, binary.width()};
}


BitVector grayToBinary(const BitVector& gray) {
    BigInt binary = 0;
    bool current = gray.bit(gray.width() - 1U);

    if (current) {
        binary |= BigInt{1} << static_cast<unsigned int>(gray.width() - 1U);
    }

    for (std::size_t index = gray.width() - 1U; index-- > 0;) {
        current = current != gray.bit(index);
        if (current) {
            binary |= BigInt{1} << static_cast<unsigned int>(index);
        }
    }

    return BitVector{std::move(binary), gray.width()};
}


GrayResult encodeGray(BigInt value, std::size_t width) {
    if (width < 1 || width > BitVector::kMaxWidth) {
        throw std::invalid_argument("Gray code width must be 1-64 bits");
    }

    if (value < 0) {
        throw RangeError("Gray code represents unsigned values only");
    }

    const BigInt maximum = (BigInt{1} << static_cast<unsigned int>(width)) - 1;
    if (value > maximum) {
        throw RangeError(
            "Value " + value.str() + " is outside the Gray code range [0, " +
            maximum.str() + "]"
        );
    }

    BitVector binary{value, width};
    BitVector gray = binaryToGray(binary);

    return GrayResult{
        .width = width,
        .value = std::move(value),
        .binary = std::move(binary),
        .gray = std::move(gray)
    };
}


GrayResult decodeGray(const BitVector& gray) {
    BitVector binary = grayToBinary(gray);
    return GrayResult{
        .width = gray.width(),
        .value = binary.toUnsigned(),
        .binary = std::move(binary),
        .gray = gray
    };
}

} // namespace radixlab::encoding
