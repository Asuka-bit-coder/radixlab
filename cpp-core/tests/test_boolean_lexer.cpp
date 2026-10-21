#include "radixlab/boolean/boolean_error.hpp"
#include "radixlab/boolean/lexer.hpp"
#include "test_support.hpp"

#include <string>

using radixlab::boolean::BooleanLimitError;
using radixlab::boolean::Lexer;
using radixlab::boolean::ParseError;
using radixlab::boolean::TokenType;
using radixlab::test::Suite;

int main() {
    Suite suite("Boolean Lexer Tests");

    const auto tokens = Lexer::tokenize("A & !B | (C ^ 1)");
    const TokenType expected[] = {
        TokenType::Variable,
        TokenType::And,
        TokenType::Not,
        TokenType::Variable,
        TokenType::Or,
        TokenType::LeftParen,
        TokenType::Variable,
        TokenType::Xor,
        TokenType::Constant,
        TokenType::RightParen,
        TokenType::End
    };

    bool typesMatch = tokens.size() == 11;
    for (std::size_t index = 0; index < tokens.size() && index < 11; ++index) {
        if (tokens[index].type != expected[index]) {
            typesMatch = false;
        }
    }
    suite.check("token classification", typesMatch);
    suite.check("variable text", tokens[0].text == 'A');
    suite.check("first operator position", tokens[1].position == 2);
    suite.check("NOT position", tokens[2].position == 4);
    suite.check("constant text", tokens[8].text == '1');
    suite.check("end position", tokens.back().position == 16);

    const auto spaced = Lexer::tokenize("  A\n\t&\r\nB  ");
    suite.check(
        "whitespace and newlines ignored",
        spaced.size() == 4
            && spaced[0].type == TokenType::Variable
            && spaced[1].type == TokenType::And
            && spaced[2].type == TokenType::Variable
            && spaced[3].type == TokenType::End
    );

    const auto empty = Lexer::tokenize("");
    suite.check("empty input yields end token",
        empty.size() == 1 && empty[0].type == TokenType::End);

    suite.expectThrows<ParseError>("lowercase variable rejected", [] {
        (void)Lexer::tokenize("a & B");
    });
    try {
        (void)Lexer::tokenize("a & B");
        suite.check("lowercase error position", false);
    } catch (const ParseError& error) {
        suite.check(
            "lowercase error position",
            error.position() == 0
                && std::string(error.what()).find("uppercase") != std::string::npos
        );
    }

    suite.expectThrows<ParseError>("unexpected character rejected", [] {
        (void)Lexer::tokenize("A ? B");
    });
    try {
        (void)Lexer::tokenize("A ? B");
        suite.check("unexpected character position", false);
    } catch (const ParseError& error) {
        suite.check(
            "unexpected character position",
            error.position() == 2
        );
    }

    const std::string oversized(4097, 'A');
    suite.expectThrows<BooleanLimitError>("oversized expression rejected", [&] {
        (void)Lexer::tokenize(oversized);
    });

    suite.check(
        "token type names",
        radixlab::boolean::tokenTypeName(TokenType::And) == "&"
            && radixlab::boolean::tokenTypeName(TokenType::End) == "end of input"
    );

    return suite.finish();
}
