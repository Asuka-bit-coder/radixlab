#include "radixlab/boolean/boolean_error.hpp"
#include "radixlab/boolean/canonical_forms.hpp"
#include "radixlab/boolean/equivalence.hpp"
#include "radixlab/boolean/evaluator.hpp"
#include "radixlab/boolean/parser.hpp"
#include "radixlab/boolean/truth_table.hpp"
#include "test_support.hpp"

#include <string>

using radixlab::boolean::BooleanLimitError;
using radixlab::boolean::checkEquivalent;
using radixlab::boolean::evaluate;
using radixlab::boolean::generateCanonicalForms;
using radixlab::boolean::generateTruthTable;
using radixlab::boolean::Parser;
using radixlab::test::Suite;

int main() {
    Suite suite("Boolean Truth Table Tests");

    const auto andTable = generateTruthTable(*Parser::parse("A & B"));
    suite.check("two variables sorted", andTable.variables.size() == 2
        && andTable.variables[0] == 'A'
        && andTable.variables[1] == 'B');
    suite.check("two-variable row count", andTable.rows.size() == 4);
    suite.check(
        "row ordering and AND output",
        andTable.rows[0].index == 0
            && !andTable.rows[0].assignment.at('A')
            && !andTable.rows[0].assignment.at('B')
            && !andTable.rows[0].result
            && andTable.rows[1].index == 1
            && !andTable.rows[1].assignment.at('A')
            && andTable.rows[1].assignment.at('B')
            && !andTable.rows[1].result
            && andTable.rows[2].index == 2
            && andTable.rows[2].assignment.at('A')
            && !andTable.rows[2].assignment.at('B')
            && !andTable.rows[2].result
            && andTable.rows[3].result
    );

    const auto sortedVariables = generateTruthTable(*Parser::parse("B | A"));
    suite.check(
        "variables are alphabetically sorted",
        sortedVariables.variables[0] == 'A'
            && sortedVariables.variables[1] == 'B'
    );

    const auto constantOne = generateTruthTable(*Parser::parse("1"));
    suite.check(
        "constant truth table has one empty row",
        constantOne.variables.empty()
            && constantOne.rows.size() == 1
            && constantOne.rows[0].assignment.empty()
            && constantOne.rows[0].result
    );

    const auto constantZero = generateTruthTable(*Parser::parse("0"));
    suite.check(
        "constant zero table",
        constantZero.rows.size() == 1 && !constantZero.rows[0].result
    );

    suite.expectThrows<BooleanLimitError>("seven variables rejected", [] {
        (void)generateTruthTable(
            *Parser::parse("A | B | C | D | E | F | G")
        );
    });

    const auto xorCanonical = generateCanonicalForms(*Parser::parse("A ^ B"));
    suite.check(
        "XOR minterms",
        xorCanonical.minterms == std::vector<std::size_t>{1, 2}
    );
    suite.check(
        "XOR maxterms",
        xorCanonical.maxterms == std::vector<std::size_t>{0, 3}
    );
    suite.check(
        "XOR canonical SOP",
        xorCanonical.sop == "(!A & B) | (A & !B)"
    );
    suite.check(
        "XOR canonical POS",
        xorCanonical.pos == "(A | B) & (!A | !B)"
    );
    suite.check(
        "canonical SOP is equivalent",
        checkEquivalent(*Parser::parse("A ^ B"), *Parser::parse(xorCanonical.sop))
            .equivalent
    );
    suite.check(
        "canonical POS is equivalent",
        checkEquivalent(*Parser::parse("A ^ B"), *Parser::parse(xorCanonical.pos))
            .equivalent
    );

    const auto constantOneForms = generateCanonicalForms(*Parser::parse("1"));
    suite.check(
        "constant one canonical forms",
        constantOneForms.sop == "1"
            && constantOneForms.pos == "1"
            && constantOneForms.minterms.size() == 1
            && constantOneForms.maxterms.empty()
    );

    const auto constantZeroForms = generateCanonicalForms(*Parser::parse("0"));
    suite.check(
        "constant zero canonical forms",
        constantZeroForms.sop == "0"
            && constantZeroForms.pos == "0"
            && constantZeroForms.minterms.empty()
            && constantZeroForms.maxterms.size() == 1
    );

    const auto tautology = generateCanonicalForms(*Parser::parse("A | !A"));
    suite.check(
        "tautology canonical expansion",
        tautology.sop == "(!A) | (A)"
            && tautology.pos == "1"
            && tautology.maxterms.empty()
    );

    const auto contradiction = generateCanonicalForms(*Parser::parse("A & !A"));
    suite.check(
        "contradiction canonical expansion",
        contradiction.sop == "0"
            && contradiction.pos == "(A) & (!A)"
            && contradiction.minterms.empty()
    );

    const auto singleVariable = generateCanonicalForms(*Parser::parse("!A"));
    suite.check(
        "single-variable canonical forms",
        singleVariable.sop == "(!A)"
            && singleVariable.pos == "(!A)"
    );

    return suite.finish();
}
