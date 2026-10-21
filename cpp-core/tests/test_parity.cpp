#include "radixlab/encoding/parity.hpp"
#include "test_support.hpp"

#include <stdexcept>

using radixlab::encoding::BitVector;
using radixlab::encoding::generateParity;
using radixlab::encoding::ParityMode;
using radixlab::encoding::parseParityMode;
using radixlab::encoding::verifyParity;
using radixlab::test::Suite;

int main() {
    Suite suite("Parity Tests");

    const BitVector data = BitVector::fromBinary("1011001");

    const auto even = generateParity(data, ParityMode::Even);
    suite.check("even parity one count", even.onesCount == 4);
    suite.check("even parity bit", !even.parityBit);
    suite.check("even parity word", even.dataWithParity == "10110010");
    suite.check(
        "even parity verifies",
        verifyParity(data, even.parityBit, ParityMode::Even)
    );

    const auto odd = generateParity(data, ParityMode::Odd);
    suite.check("odd parity bit", odd.parityBit);
    suite.check("odd parity word", odd.dataWithParity == "10110011");
    suite.check(
        "odd parity verifies",
        verifyParity(data, odd.parityBit, ParityMode::Odd)
    );

    const auto singleFlip = data.flipBit(3);
    suite.check(
        "even parity detects one flipped bit",
        !verifyParity(singleFlip, even.parityBit, ParityMode::Even)
    );
    suite.check(
        "odd parity detects one flipped bit",
        !verifyParity(singleFlip, odd.parityBit, ParityMode::Odd)
    );

    const auto doubleFlip = data.flipBit(0).flipBit(1);
    suite.check(
        "even parity can miss two flipped bits",
        verifyParity(doubleFlip, even.parityBit, ParityMode::Even)
    );
    suite.check(
        "odd parity can miss two flipped bits",
        verifyParity(doubleFlip, odd.parityBit, ParityMode::Odd)
    );

    const BitVector zeros = BitVector::zero(8);
    suite.check(
        "all-zero even parity bit",
        !generateParity(zeros, ParityMode::Even).parityBit
    );
    suite.check(
        "all-zero odd parity bit",
        generateParity(zeros, ParityMode::Odd).parityBit
    );

    const BitVector ones64{ (radixlab::BigInt{1} << 64U) - 1, 64 };
    const auto even64 = generateParity(ones64, ParityMode::Even);
    suite.check("64-bit parity one count", even64.onesCount == 64);
    suite.check("64-bit even parity bit", !even64.parityBit);
    suite.check(
        "64-bit parity word length",
        even64.dataWithParity.size() == 65
    );

    for (const std::size_t width : {4U, 8U, 16U, 32U, 64U}) {
        const BitVector zero = BitVector::zero(width);
        suite.check(
            "parity generation width " + std::to_string(width),
            verifyParity(
                zero,
                generateParity(zero, ParityMode::Even).parityBit,
                ParityMode::Even
            )
        );
    }

    suite.check(
        "parse parity names",
        parseParityMode("even") == ParityMode::Even
            && parseParityMode("ODD_PARITY") == ParityMode::Odd
    );
    suite.expectThrows<std::invalid_argument>("unknown parity rejected", [] {
        (void)parseParityMode("MARK");
    });

    return suite.finish();
}
