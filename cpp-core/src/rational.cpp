#include "radixlab/rational.hpp"

#include <stdexcept>
#include <utility>

namespace radixlab {

namespace {

BigInt absBigInt(BigInt value) {
    return value < 0 ? -value : value;
}

BigInt gcd(BigInt a, BigInt b) {
    a = absBigInt(std::move(a));
    b = absBigInt(std::move(b));

    while (b != 0) {
        BigInt remainder = a % b;
        a = std::move(b);
        b = std::move(remainder);
    }

    return a;
}

} // namespace


Rational::Rational(BigInt numerator, BigInt denominator)
    : numerator_(std::move(numerator)),
      denominator_(std::move(denominator)) {

    normalize();
}


void Rational::normalize() {

    if (denominator_ == 0) {
        throw std::invalid_argument(
            "Denominator cannot be zero"
        );
    }

    // Ensure denominator is positive.
    if (denominator_ < 0) {
        numerator_ = -numerator_;
        denominator_ = -denominator_;
    }

    // Canonical zero representation: 0/1.
    if (numerator_ == 0) {
        denominator_ = 1;
        return;
    }

    // Reduce the fraction.
    const BigInt divisor = gcd(
        numerator_,
        denominator_
    );

    numerator_ /= divisor;
    denominator_ /= divisor;
}


const BigInt& Rational::numerator() const noexcept {
    return numerator_;
}


const BigInt& Rational::denominator() const noexcept {
    return denominator_;
}


std::string Rational::toString() const {

    const std::string num =
        numerator_.str();

    if (denominator_ == 1) {
        return num;
    }

    return num + "/" + denominator_.str();
}

} // namespace radixlab
