#include "radixlab/boolean/ast.hpp"
#include "radixlab/boolean/boolean_error.hpp"
#include "radixlab/boolean/evaluator.hpp"
#include "radixlab/boolean/limits.hpp"
#include "radixlab/boolean/parser.hpp"
#include "test_support.hpp"

#include <string>

using radixlab::boolean::Assignment;
using radixlab::boolean::BinaryExpr;
using radixlab::boolean::BinaryOperator;
using radixlab::boolean::BooleanLimitError;
using radixlab::boolean::evaluate;
using radixlab::boolean::ExprPtr;
using radixlab::boolean::NotExpr;
using radixlab::boolean::ParseError;
using radixlab::boolean::Parser;
using radixlab::boolean::VariableExpr;
using radixlab::test::Suite;

namespace {

void expectParseError(
    Suite& suite,
    std::string_view name,
    std::string_view expression,
    std::size_t position
) {
    try {
        (void)Parser::parse(expression);
        suite.check(name, false);
    } catch (const ParseError& error) {
        suite.check(name, error.position() == position);
    } catch (...) {
        suite.check(name, false);
    }
}

} // namespace

int main() {
    Suite suite("Boolean Parser Tests");

    const ExprPtr precedence = Parser::parse("A | B & C ^ !D");
    suite.check(
        "precedence display",
        precedence->toString() == "(A | ((B & C) ^ !D))"
    );

    Assignment precedenceAssignment{
        {'A', false}, {'B', true}, {'C', true}, {'D', true}
    };
    suite.check(
        "precedence evaluation",
        evaluate(*precedence, precedenceAssignment)
    );

    const ExprPtr leftAssociative = Parser::parse("A | B | C");
    suite.check(
        "binary operators are left-associative",
        leftAssociative->toString() == "((A | B) | C)"
    );

    const ExprPtr grouped = Parser::parse("A & (B | C)");
    const auto* groupedRoot = dynamic_cast<const BinaryExpr*>(grouped.get());
    suite.check(
        "parentheses build expected AST",
        groupedRoot != nullptr
            && groupedRoot->op == BinaryOperator::And
            && dynamic_cast<const BinaryExpr*>(groupedRoot->right.get()) != nullptr
    );

    const ExprPtr nestedNot = Parser::parse("!!A");
    suite.check(
        "nested NOT node",
        dynamic_cast<const NotExpr*>(nestedNot.get()) != nullptr
    );

    Assignment trueAssignment{{'A', true}};
    suite.check(
        "double NOT evaluates to original value",
        evaluate(*nestedNot, trueAssignment)
    );

    const ExprPtr variable = Parser::parse("A");
    suite.check(
        "variable AST JSON",
        variable->toJson() == "{\"type\":\"variable\",\"name\":\"A\"}"
    );

    const ExprPtr json = Parser::parse("A & 1");
    suite.check(
        "operator AST JSON",
        json->toJson().find("\"type\":\"and\"") != std::string::npos
            && json->toJson().find("\"type\":\"constant\"") != std::string::npos
    );

    const ExprPtr noVariables = Parser::parse("1");
    suite.check(
        "constant expression",
        evaluate(*noVariables, Assignment{})
    );

    expectParseError(suite, "empty expression", "", 0);
    expectParseError(suite, "consecutive binary operators", "A & | B", 4);
    expectParseError(suite, "missing closing parenthesis", "(A & B", 6);
    expectParseError(suite, "extra closing parenthesis", "A)", 1);
    expectParseError(suite, "implicit multiplication", "AB", 1);
    expectParseError(suite, "trailing operator", "A &", 3);
    expectParseError(suite, "group without expression", "()", 1);

    std::string deep;
    deep.reserve(131);
    for (std::size_t index = 0; index <= radixlab::boolean::kMaxNestingDepth; ++index) {
        deep.push_back('(');
    }
    deep += 'A';
    for (std::size_t index = 0; index <= radixlab::boolean::kMaxNestingDepth; ++index) {
        deep.push_back(')');
    }
    suite.expectThrows<BooleanLimitError>("nesting depth limit", [&] {
        (void)Parser::parse(deep);
    });

    std::string oversized(4100, 'A');
    suite.expectThrows<BooleanLimitError>("parser length limit", [&] {
        (void)Parser::parse(oversized);
    });

    return suite.finish();
}
