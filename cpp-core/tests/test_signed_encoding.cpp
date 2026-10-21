#include "radixlab/encoding/encoding_error.hpp"
#include "radixlab/encoding/signed_encoding.hpp"
#include "test_support.hpp"

#include <stdexcept>
#include <string>

using radixlab::BigInt;
using radixlab::encoding::decodeSigned;
using radixlab::encoding::encodeSigned;
using radixlab::encoding::parseSignedEncoding;
using radixlab::encoding::RangeError;
using radixlab::encoding::SignedEncoding;
using radixlab::encoding::signedRange;
using radixlab::test::Suite;

namespace {

void expectEncoding(
    Suite& suite,
    std::string_view name,
    BigInt value,
    std::size_t width,
    SignedEncoding encoding,
    std::string_view expectedBits,
    bool negativeZero = false
) {
    try {
        const auto result = encodeSigned(value, width, encoding, negativeZero);
        if (result.bits.toBinary() != expectedBits
            || result.negativeZero != negativeZero) {
            suite.check(name, false);
        } else {
            suite.check(name, true);
        }
    } catch (const std::exception& error) {
        suite.check(name, false);
    }
}

void expectDecoded(
    Suite& suite,
    std::string_view name,
    std::string_view bits,
    SignedEncoding encoding,
    std::string_view expectedValue,
    bool expectedNegativeZero = false
) {
    try {
        const auto result = decodeSigned(
            radixlab::encoding::BitVector::fromBinary(bits),
            encoding
        );
        suite.check(
            name,
            result.value.str() == expectedValue
                && result.negativeZero == expectedNegativeZero
        );
    } catch (const std::exception& error) {
        suite.check(name, false);
    }
}

} // namespace

int main() {
    Suite suite("Signed Encoding Tests");

    constexpr SignedEncoding sm = SignedEncoding::SignMagnitude;
    constexpr SignedEncoding ones = SignedEncoding::OnesComplement;
    constexpr SignedEncoding twos = SignedEncoding::TwosComplement;

    expectEncoding(suite, "+13 sign-magnitude", 13, 8, sm, "00001101");
    expectEncoding(suite, "-13 sign-magnitude", -13, 8, sm, "10001101");
    expectEncoding(suite, "+13 ones' complement", 13, 8, ones, "00001101");
    expectEncoding(suite, "-13 ones' complement", -13, 8, ones, "11110010");
    expectEncoding(suite, "+13 two's complement", 13, 8, twos, "00001101");
    expectEncoding(suite, "-13 two's complement", -13, 8, twos, "11110011");

    expectEncoding(suite, "+127 sign-magnitude", 127, 8, sm, "01111111");
    expectEncoding(suite, "-127 sign-magnitude", -127, 8, sm, "11111111");
    expectEncoding(suite, "+127 ones' complement", 127, 8, ones, "01111111");
    expectEncoding(suite, "-127 ones' complement", -127, 8, ones, "10000000");
    expectEncoding(suite, "+127 two's complement", 127, 8, twos, "01111111");
    expectEncoding(suite, "-128 two's complement", -128, 8, twos, "10000000");

    expectEncoding(suite, "+0 all encodings", 0, 8, sm, "00000000");
    expectEncoding(suite, "negative zero sign-magnitude", 0, 8, sm, "10000000", true);
    expectEncoding(suite, "negative zero ones' complement", 0, 8, ones, "11111111", true);

    expectDecoded(suite, "decode -13 sign-magnitude", "10001101", sm, "-13");
    expectDecoded(suite, "decode -13 ones' complement", "11110010", ones, "-13");
    expectDecoded(suite, "decode -13 two's complement", "11110011", twos, "-13");
    expectDecoded(suite, "decode -128 two's complement", "10000000", twos, "-128");
    expectDecoded(suite, "decode negative zero sign-magnitude", "10000000", sm, "0", true);
    expectDecoded(suite, "decode negative zero ones' complement", "11111111", ones, "0", true);
    expectDecoded(suite, "decode sign-magnitude positive", "01111111", sm, "127");
    expectDecoded(suite, "decode ones' complement positive", "01111111", ones, "127");

    suite.expectThrows<RangeError>("sign-magnitude -128 rejected", [] {
        (void)encodeSigned(-128, 8, SignedEncoding::SignMagnitude);
    });
    suite.expectThrows<RangeError>("ones' complement -128 rejected", [] {
        (void)encodeSigned(-128, 8, SignedEncoding::OnesComplement);
    });
    suite.expectThrows<RangeError>("positive 128 rejected", [] {
        (void)encodeSigned(128, 8, SignedEncoding::TwosComplement);
    });
    suite.expectThrows<RangeError>("two's complement -129 rejected", [] {
        (void)encodeSigned(-129, 8, SignedEncoding::TwosComplement);
    });
    suite.expectThrows<std::invalid_argument>("negative zero in two's complement", [] {
        (void)encodeSigned(0, 8, SignedEncoding::TwosComplement, true);
    });
    suite.expectThrows<std::invalid_argument>("negative zero with nonzero value", [] {
        (void)encodeSigned(1, 8, SignedEncoding::SignMagnitude, true);
    });
    suite.expectThrows<std::invalid_argument>("zero width rejected", [] {
        (void)signedRange(0, SignedEncoding::TwosComplement);
    });
    suite.expectThrows<std::invalid_argument>("65-bit width rejected", [] {
        (void)signedRange(65, SignedEncoding::TwosComplement);
    });

    for (const std::size_t width : {4U, 8U, 16U, 32U, 64U}) {
        const BigInt maximum = (BigInt{1} << static_cast<unsigned>(width - 1U)) - 1;
        const BigInt minimumTwos = -maximum - 1;

        const auto maximumEncoding = encodeSigned(maximum, width, twos);
        const auto minimumEncoding = encodeSigned(minimumTwos, width, twos);
        const auto maximumDecoded = decodeSigned(maximumEncoding.bits, twos);
        const auto minimumDecoded = decodeSigned(minimumEncoding.bits, twos);

        suite.check(
            "two's complement round trip width " + std::to_string(width),
            maximumDecoded.value == maximum
                && minimumDecoded.value == minimumTwos
        );
        suite.check(
            "two's complement sign bit width " + std::to_string(width),
            !maximumEncoding.bits.signBit() && minimumEncoding.bits.signBit()
        );

        const auto range = signedRange(width, sm);
        suite.check(
            "sign-magnitude range width " + std::to_string(width),
            range.minimum == -maximum && range.maximum == maximum
        );
    }

    const auto fourBitMinimum = encodeSigned(-8, 4, twos);
    suite.check("4-bit two's complement minimum", fourBitMinimum.bits.toBinary() == "1000");

    const auto smNegativeZero = encodeSigned(0, 4, sm, true);
    suite.check("4-bit negative zero", smNegativeZero.bits.toBinary() == "1000");

    const auto onesNegativeZero = encodeSigned(0, 16, ones, true);
    suite.check(
        "16-bit negative zero",
        onesNegativeZero.bits.toBinary() == "1111111111111111"
    );

    const BigInt max64 = (BigInt{1} << 63U) - 1;
    const BigInt min64 = -(BigInt{1} << 63U);
    suite.check(
        "64-bit two's complement range",
        encodeSigned(max64, 64, twos).bits.toHex() == "7FFFFFFFFFFFFFFF"
            && encodeSigned(min64, 64, twos).bits.toHex() == "8000000000000000"
    );

    suite.check(
        "parse encoding names",
        parseSignedEncoding("sign-magnitude") == sm
            && parseSignedEncoding("ONES_COMPLEMENT") == ones
            && parseSignedEncoding("twos complement") == twos
    );
    suite.expectThrows<std::invalid_argument>("unknown encoding rejected", [] {
        (void)parseSignedEncoding("excess-127");
    });

    return suite.finish();
}
