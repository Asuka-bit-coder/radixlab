#include "radixlab/boolean/boolean_error.hpp"
#include "radixlab/boolean/equivalence.hpp"
#include "radixlab/boolean/parser.hpp"
#include "test_support.hpp"

#include <string>

using radixlab::boolean::BooleanLimitError;
using radixlab::boolean::checkEquivalent;
using radixlab::boolean::Parser;
using radixlab::test::Suite;

namespace {

void expectEquivalent(
    Suite& suite,
    std::string_view name,
    std::string_view left,
    std::string_view right
) {
    try {
        const auto result = checkEquivalent(
            *Parser::parse(left),
            *Parser::parse(right)
        );
        suite.check(name, result.equivalent);
    } catch (...) {
        suite.check(name, false);
    }
}

} // namespace

int main() {
    Suite suite("Boolean Equivalence Tests");

    expectEquivalent(suite, "identity AND", "A", "A & 1");
    expectEquivalent(suite, "identity OR", "A", "A | 0");
    expectEquivalent(suite, "domination AND", "A & 0", "0");
    expectEquivalent(suite, "domination OR", "A | 1", "1");
    expectEquivalent(suite, "idempotent AND", "A", "A & A");
    expectEquivalent(suite, "idempotent OR", "A", "A | A");
    expectEquivalent(suite, "complement AND", "A & !A", "0");
    expectEquivalent(suite, "complement OR", "A | !A", "1");
    expectEquivalent(suite, "involution", "A", "!!A");
    expectEquivalent(suite, "commutative AND", "A & B", "B & A");
    expectEquivalent(suite, "commutative OR", "A | B", "B | A");
    expectEquivalent(suite, "commutative XOR", "A ^ B", "B ^ A");
    expectEquivalent(suite, "associative AND", "(A & B) & C", "A & (B & C)");
    expectEquivalent(suite, "associative OR", "(A | B) | C", "A | (B | C)");
    expectEquivalent(
        suite,
        "distributive AND over OR",
        "A & (B | C)",
        "(A & B) | (A & C)"
    );
    expectEquivalent(
        suite,
        "distributive OR over AND",
        "A | (B & C)",
        "(A | B) & (A | C)"
    );
    expectEquivalent(suite, "absorption OR", "A | (A & B)", "A");
    expectEquivalent(suite, "absorption AND", "A & (A | B)", "A");
    expectEquivalent(suite, "De Morgan AND", "!(A & B)", "!A | !B");
    expectEquivalent(suite, "De Morgan OR", "!(A | B)", "!A & !B");
    expectEquivalent(
        suite,
        "four-variable distributivity",
        "A & (B | C | D)",
        "(A & B) | (A & C) | (A & D)"
    );

    const auto nonEquivalent = checkEquivalent(
        *Parser::parse("A & B"),
        *Parser::parse("A | B")
    );
    suite.check("AND and OR are not equivalent", !nonEquivalent.equivalent);
    suite.check(
        "counterexample is present",
        nonEquivalent.counterexample.has_value()
    );
    if (nonEquivalent.counterexample.has_value()) {
        const auto& counterexample = *nonEquivalent.counterexample;
        suite.check(
            "counterexample has expected assignment",
            counterexample.assignment.at('A') == false
                && counterexample.assignment.at('B') == true
                && counterexample.leftResult == false
                && counterexample.rightResult == true
        );
    }

    const auto differentVariables = checkEquivalent(
        *Parser::parse("A"),
        *Parser::parse("B")
    );
    suite.check(
        "different variables are compared over their union",
        !differentVariables.equivalent
            && differentVariables.variables
                == std::vector<char>{'A', 'B'}
    );

    expectEquivalent(suite, "constants equal", "1", "!(0)");
    expectEquivalent(suite, "constant constants", "0", "A & !A");

    suite.expectThrows<BooleanLimitError>("equivalence variable limit", [] {
        (void)checkEquivalent(
            *Parser::parse("A | B | C | D | E | F | G"),
            *Parser::parse("1")
        );
    });

    return suite.finish();
}
