#pragma once

#include "radixlab/big_int.hpp"

#include <string>

namespace radixlab {

class Rational {
public:
    Rational(BigInt numerator, BigInt denominator = 1);

    [[nodiscard]]
    const BigInt& numerator() const noexcept;

    [[nodiscard]]
    const BigInt& denominator() const noexcept;

    [[nodiscard]]
    std::string toString() const;

private:
    BigInt numerator_;
    BigInt denominator_;

    void normalize();
};

} // namespace radixlab
