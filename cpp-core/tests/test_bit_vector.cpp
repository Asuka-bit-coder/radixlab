#include "radixlab/encoding/bit_vector.hpp"
#include "test_support.hpp"

#include <stdexcept>

using radixlab::BigInt;
using radixlab::encoding::BitVector;
using radixlab::encoding::countSetBits;
using radixlab::test::Suite;

int main() {
    Suite suite("BitVector Tests");

    const BitVector nibble{BigInt{0xA}, 4};
    suite.check("4-bit binary", nibble.toBinary() == "1010");
    suite.check("4-bit hexadecimal", nibble.toHex() == "A");
    suite.check("4-bit unsigned value", nibble.toUnsigned() == 10);
    suite.check("LSB bit read", nibble.bit(1));
    suite.check("MSB bit read", nibble.bit(3));

    for (const std::size_t width : {4U, 8U, 16U, 32U, 64U}) {
        const BigInt maximum = (BigInt{1} << static_cast<unsigned>(width)) - 1;
        const BitVector allOnes{maximum, width};
        suite.check(
            "width " + std::to_string(width) + " binary length",
            allOnes.toBinary().size() == width
        );
        suite.check(
            "width " + std::to_string(width) + " hex length",
            allOnes.toHex().size() == (width + 3U) / 4U
        );
        suite.check(
            "width " + std::to_string(width) + " sign bit",
            allOnes.signBit()
        );
        suite.check(
            "width " + std::to_string(width) + " popcount",
            countSetBits(allOnes) == width
        );
    }

    const BitVector byte = BitVector::fromBinary("10010110");
    suite.check("binary parse width", byte.width() == 8);
    suite.check("binary parse value", byte.toUnsigned() == 150);
    suite.check("binary parse round trip", byte.toBinary() == "10010110");
    suite.check("hex parse", BitVector::fromHex("ff", 8).toHex() == "FF");
    suite.check(
        "hex parse round trip",
        BitVector::fromHex("5A", 8).toBinary() == "01011010"
    );
    suite.check("zero factory", BitVector::zero(16).toBinary() == "0000000000000000");

    suite.check("left shift", byte.shiftLeft(1).toBinary() == "00101100");
    suite.check("right shift", byte.shiftRight(1).toBinary() == "01001011");
    suite.check("left shift beyond width", byte.shiftLeft(8).toBinary() == "00000000");
    suite.check("right shift beyond width", byte.shiftRight(99).toBinary() == "00000000");

    const BitVector topBit{BigInt{1} << 63U, 64};
    suite.check("64-bit left shift discards top bit", topBit.shiftLeft(1) == BitVector::zero(64));
    suite.check("64-bit right shift boundary", topBit.shiftRight(63) == BitVector{BigInt{1}, 64});

    const BitVector lowNibble{BigInt{0x3}, 4};
    const BitVector highNibble{BigInt{0x8}, 4};
    suite.check("bitwise AND", lowNibble.bitwiseAnd(highNibble).toHex() == "0");
    suite.check("bitwise OR", lowNibble.bitwiseOr(highNibble).toHex() == "B");
    suite.check("bitwise XOR", lowNibble.bitwiseXor(highNibble).toHex() == "B");
    suite.check("bitwise NOT masks width", lowNibble.bitwiseNot().toHex() == "C");
    suite.check("withBit set", lowNibble.withBit(3, true).toHex() == "B");
    suite.check("withBit clear", highNibble.withBit(3, false).toHex() == "0");
    suite.check("flipBit", lowNibble.flipBit(3).toHex() == "B");

    suite.expectThrows<std::invalid_argument>("zero width rejected", [] {
        (void)BitVector{BigInt{0}, 0};
    });
    suite.expectThrows<std::invalid_argument>("width 65 rejected", [] {
        (void)BitVector{BigInt{0}, 65};
    });
    suite.expectThrows<std::invalid_argument>("negative pattern rejected", [] {
        (void)BitVector{BigInt{-1}, 8};
    });
    suite.expectThrows<std::invalid_argument>("pattern overflow rejected", [] {
        (void)BitVector{BigInt{16}, 4};
    });
    suite.expectThrows<std::invalid_argument>("empty binary rejected", [] {
        (void)BitVector::fromBinary("");
    });
    suite.expectThrows<std::invalid_argument>("invalid binary rejected", [] {
        (void)BitVector::fromBinary("102");
    });
    suite.expectThrows<std::invalid_argument>("65-bit input rejected", [] {
        (void)BitVector::fromBinary(
            "10000000000000000000000000000000000000000000000000000000000000000"
        );
    });
    suite.expectThrows<std::invalid_argument>("hex overflow rejected", [] {
        (void)BitVector::fromHex("FF", 4);
    });
    suite.expectThrows<std::invalid_argument>("non-hex digit rejected", [] {
        (void)BitVector::fromHex("G", 8);
    });
    suite.expectThrows<std::out_of_range>("bit index rejected", [&byte] {
        (void)byte.bit(8);
    });
    suite.expectThrows<std::invalid_argument>("mixed width AND rejected", [] {
        const BitVector a{BigInt{1}, 4};
        const BitVector b{BigInt{1}, 8};
        (void)a.bitwiseAnd(b);
    });
    suite.expectThrows<std::out_of_range>("flip index rejected", [&byte] {
        (void)byte.flipBit(8);
    });

    return suite.finish();
}
