#include "radixlab/rational.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

using radixlab::Rational;
using radixlab::BigInt;

namespace {

int passed = 0;
int failed = 0;

void check(std::string_view name, bool condition) {

    if (condition) {
        ++passed;
        std::cout << "[PASS] " << name << '\n';
    } else {
        ++failed;
        std::cerr << "[FAIL] " << name << '\n';
    }
}

} // namespace

int main() {

    // Basic fractions
    Rational a{2, 4};

    check(
        "Fraction reduction",
        a.numerator() == 1 &&
        a.denominator() == 2
    );

    // Negative numerator
    Rational b{-2, 4};

    check(
        "Negative numerator",
        b.toString() == "-1/2"
    );

    // Negative denominator
    Rational c{2, -4};

    check(
        "Negative denominator",
        c.toString() == "-1/2"
    );

    // Double negative
    Rational d{-2, -4};

    check(
        "Double negative",
        d.toString() == "1/2"
    );

    // Zero
    Rational e{0, 100};

    check(
        "Zero normalization",
        e.toString() == "0" &&
        e.denominator() == 1
    );

    // Integer
    Rational f{42};

    check(
        "Integer representation",
        f.toString() == "42"
    );

    // Exact decimal fraction
    Rational g{1, 10};

    check(
        "Exact decimal fraction",
        g.toString() == "1/10"
    );

    // Large integers
    BigInt large =
        BigInt{"123456789012345678901234567890"};

    Rational h{large * 3, 3};

    check(
        "Arbitrary precision",
        h.numerator() == large &&
        h.denominator() == 1
    );

    // Invalid denominator
    bool rejected = false;

    try {
        Rational invalid{1, 0};
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    check(
        "Zero denominator rejected",
        rejected
    );

    std::cout << "\nPassed: " << passed
              << "\nFailed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
