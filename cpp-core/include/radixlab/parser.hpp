#pragma once

#include <cstdint>
#include <string_view>

namespace radixlab {

class RadixParser {
public:
    /**
     * Parse an integer numeral in the specified radix.
     *
     * @param input Numeral string, optionally prefixed with + or -.
     * @param radix Base between 2 and 36.
     * @return Parsed signed 64-bit integer.
     *
     * @throws std::invalid_argument For invalid input or radix.
     * @throws std::out_of_range If the value exceeds int64_t range.
     */
    [[nodiscard]]
    static std::int64_t parse(
        std::string_view input,
        int radix
    );
};

} // namespace radixlab
