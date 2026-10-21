#include "radixlab/boolean/boolean_error.hpp"
#include "radixlab/boolean/evaluator.hpp"
#include "radixlab/boolean/parser.hpp"
#include "test_support.hpp"

using radixlab::boolean::Assignment;
using radixlab::boolean::EvaluationError;
using radixlab::boolean::evaluate;
using radixlab::boolean::Parser;
using radixlab::test::Suite;

int main() {
    Suite suite("Boolean Evaluator Tests");

    const auto expression = Parser::parse("A & (!B | C)");

    suite.check(
        "course example",
        evaluate(
            *expression,
            Assignment{{'A', true}, {'B', false}, {'C', false}}
        )
    );
    suite.check(
        "true via disjunction",
        evaluate(
            *expression,
            Assignment{{'A', true}, {'B', false}, {'C', true}}
        )
    );
    suite.check(
        "false via A",
        !evaluate(
            *expression,
            Assignment{{'A', false}, {'B', true}, {'C', false}}
        )
    );
    suite.check(
        "extra assignment entries are ignored",
        evaluate(
            *expression,
            Assignment{{'A', true}, {'B', false}, {'C', false}, {'Z', true}}
        )
    );

    suite.expectThrows<EvaluationError>("missing variable rejected", [&] {
        (void)evaluate(*expression, Assignment{{'A', true}});
    });
    try {
        (void)evaluate(*expression, Assignment{{'A', true}});
        suite.check("missing variable message", false);
    } catch (const EvaluationError& error) {
        suite.check(
            "missing variable message",
            std::string(error.what()).find("variable B") != std::string::npos
                || std::string(error.what()).find("variable C") != std::string::npos
        );
    }

    const auto constantFalse = Parser::parse("0");
    const auto constantTrue = Parser::parse("1");
    suite.check("constant false", !evaluate(*constantFalse, Assignment{}));
    suite.check("constant true", evaluate(*constantTrue, Assignment{}));

    const auto xorExpression = Parser::parse("A ^ B");
    suite.check(
        "XOR differs",
        evaluate(*xorExpression, Assignment{{'A', true}, {'B', false}})
            && !evaluate(*xorExpression, Assignment{{'A', true}, {'B', true}})
    );

    return suite.finish();
}
