#include "radixlab/encoding/encoding_error.hpp"
#include "radixlab/encoding/gray_code.hpp"
#include "test_support.hpp"

#include <stdexcept>
#include <string>

using radixlab::BigInt;
using radixlab::encoding::binaryToGray;
using radixlab::encoding::BitVector;
using radixlab::encoding::countSetBits;
using radixlab::encoding::decodeGray;
using radixlab::encoding::encodeGray;
using radixlab::encoding::RangeError;
using radixlab::test::Suite;

int main() {
    Suite suite("Gray Code Tests");

    const auto thirteen = encodeGray(13, 8);
    suite.check("13 binary", thirteen.binary.toBinary() == "00001101");
    suite.check("13 Gray code", thirteen.gray.toBinary() == "00001011");
    suite.check("13 Gray hex", thirteen.gray.toHex() == "0B");

    const auto decoded = decodeGray(thirteen.gray);
    suite.check("13 Gray decode", decoded.value == 13);
    suite.check("13 Gray decode binary", decoded.binary.toBinary() == "00001101");

    const std::string expectedFourBitSequence[] = {
        "0000", "0001", "0011", "0010",
        "0110", "0111", "0101", "0100",
        "1100", "1101", "1111", "1110",
        "1010", "1011", "1001", "1000"
    };

    bool sequenceMatches = true;
    for (std::size_t value = 0; value < 16; ++value) {
        if (encodeGray(value, 4).gray.toBinary()
            != expectedFourBitSequence[value]) {
            sequenceMatches = false;
        }
    }
    suite.check("4-bit reflected Gray sequence", sequenceMatches);

    bool roundTrip = true;
    bool singleBitTransitions = true;
    for (const std::size_t width : {4U, 8U}) {
        const std::size_t limit = std::size_t{1} << width;
        BitVector previous = encodeGray(0, width).gray;

        for (std::size_t value = 0; value < limit; ++value) {
            const auto encoded = encodeGray(value, width);
            if (decodeGray(encoded.gray).value != value) {
                roundTrip = false;
            }

            if (value > 0
                && countSetBits(previous.bitwiseXor(encoded.gray)) != 1) {
                singleBitTransitions = false;
            }

            previous = encoded.gray;
        }
    }
    suite.check("Gray round trip widths 4 and 8", roundTrip);
    suite.check("adjacent Gray codes differ by one bit", singleBitTransitions);

    const auto direct = binaryToGray(BitVector::fromBinary("1101"));
    suite.check("direct binaryToGray", direct.toBinary() == "1011");
    suite.check(
        "direct grayToBinary",
        radixlab::encoding::grayToBinary(direct).toBinary() == "1101"
    );

    for (const std::size_t width : {16U, 32U}) {
        const BigInt value =
            (BigInt{1} << static_cast<unsigned>(width - 2U)) + 123;
        const auto encoded = encodeGray(value, width);
        suite.check(
            "Gray round trip width " + std::to_string(width),
            decodeGray(encoded.gray).value == value
                && encoded.gray.toBinary().size() == width
        );
    }

    const BigInt maximum64 = (BigInt{1} << 64U) - 1;
    const auto gray64 = encodeGray(maximum64, 64);
    suite.check(
        "64-bit Gray round trip",
        decodeGray(gray64.gray).value == maximum64
    );
    suite.check(
        "64-bit Gray width",
        gray64.gray.toBinary().size() == 64
    );

    suite.expectThrows<RangeError>("negative Gray value rejected", [] {
        (void)encodeGray(-1, 8);
    });
    suite.expectThrows<RangeError>("Gray value above width rejected", [] {
        (void)encodeGray(256, 8);
    });
    suite.expectThrows<std::invalid_argument>("zero Gray width rejected", [] {
        (void)encodeGray(0, 0);
    });

    return suite.finish();
}
