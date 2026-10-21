#pragma once

#include "radixlab/rational.hpp"

#include <string_view>

namespace radixlab {

class RationalParser {
public:
    /**
     * Parse a positional numeral into an exact rational.
     *
     * Supported:
     *   101.11 (base 2)
     *   -2AF.8 (base 16)
     *   0.1 (base 10)
     *   +42 (base 10)
     *
     * Radix range: [2, 36]
     *
     * Throws:
     *   std::invalid_argument for invalid input.
     */
    [[nodiscard]]
    static Rational parse(
        std::string_view input,
        int radix
    );
};

} // namespace radixlab
