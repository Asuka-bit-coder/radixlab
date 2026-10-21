#include "radixlab/encoding/bcd.hpp"
#include "test_support.hpp"

#include <stdexcept>

using radixlab::encoding::decodeBcd;
using radixlab::encoding::encodeBcd;
using radixlab::test::Suite;

int main() {
    Suite suite("BCD Tests");

    const auto fiftyNine = encodeBcd("59");
    suite.check("BCD 59 bits", fiftyNine.bits == "01011001");
    suite.check("BCD 59 groups", fiftyNine.groups.size() == 2
        && fiftyNine.groups[0] == "0101"
        && fiftyNine.groups[1] == "1001");
    suite.check("BCD 59 value", fiftyNine.value == 59);
    suite.check("BCD 59 differs from binary", fiftyNine.binary == "111011");

    const auto zero = encodeBcd("0");
    suite.check("BCD zero", zero.bits == "0000" && zero.binary == "0");

    const auto leading = encodeBcd("0059");
    suite.check("BCD leading zeros preserved", leading.digits == "0059");
    suite.check("BCD leading zeros bits", leading.bits == "0000000001011001");
    suite.check("BCD leading zeros value", leading.value == 59);

    const auto decoded = decodeBcd("0101 1001");
    suite.check("BCD decode grouped spaces", decoded.digits == "59");
    suite.check("BCD decode value", decoded.value == 59);
    suite.check("BCD decode groups", decoded.groups.size() == 2);
    suite.check("BCD decode binary comparison", decoded.binary == "111011");

    const auto decodedZeros = decodeBcd("00000000");
    suite.check("BCD decode leading zeros", decodedZeros.digits == "00");
    suite.check("BCD decode leading zeros value", decodedZeros.value == 0);

    const auto large = encodeBcd("1234567890");
    suite.check("BCD multi-digit groups", large.groups.size() == 10);
    suite.check(
        "BCD multi-digit bits length",
        large.bits.size() == 40
    );
    suite.check(
        "BCD multi-digit round trip",
        decodeBcd(large.bits).digits == "1234567890"
    );

    suite.expectThrows<std::invalid_argument>("BCD rejects negative sign", [] {
        (void)encodeBcd("-1");
    });
    suite.expectThrows<std::invalid_argument>("BCD rejects empty input", [] {
        (void)encodeBcd("");
    });
    suite.expectThrows<std::invalid_argument>("BCD rejects letters", [] {
        (void)encodeBcd("12A");
    });
    suite.expectThrows<std::invalid_argument>("BCD rejects nibble 1010", [] {
        (void)decodeBcd("1010");
    });
    suite.expectThrows<std::invalid_argument>("BCD rejects nibble 1111", [] {
        (void)decodeBcd("11110000");
    });
    suite.expectThrows<std::invalid_argument>("BCD rejects partial nibble", [] {
        (void)decodeBcd("01011");
    });
    suite.expectThrows<std::invalid_argument>("BCD rejects non-bit character", [] {
        (void)decodeBcd("01x1");
    });
    suite.expectThrows<std::invalid_argument>("BCD rejects empty decode", [] {
        (void)decodeBcd("");
    });
    suite.expectThrows<std::invalid_argument>("BCD rejects spaces only", [] {
        (void)decodeBcd("   ");
    });

    return suite.finish();
}
