#include "radixlab/boolean/lexer.hpp"

#include "radixlab/boolean/boolean_error.hpp"
#include "radixlab/boolean/limits.hpp"

#include <cctype>
#include <string>

namespace radixlab::boolean {

std::string_view tokenTypeName(TokenType type) noexcept {
    switch (type) {
        case TokenType::Variable:
            return "variable";
        case TokenType::Constant:
            return "constant";
        case TokenType::Not:
            return "!";
        case TokenType::And:
            return "&";
        case TokenType::Xor:
            return "^";
        case TokenType::Or:
            return "|";
        case TokenType::LeftParen:
            return "(";
        case TokenType::RightParen:
            return ")";
        case TokenType::End:
            return "end of input";
    }

    return "unknown token";
}


std::vector<Token> Lexer::tokenize(std::string_view expression) {
    if (expression.size() > kMaxExpressionLength) {
        throw BooleanLimitError(
            "Expression exceeds the 4096-character parser limit"
        );
    }

    std::vector<Token> tokens;
    tokens.reserve(expression.size() + 1);

    for (std::size_t position = 0; position < expression.size(); ++position) {
        const char character = expression[position];
        const unsigned char byte = static_cast<unsigned char>(character);

        if (std::isspace(byte) != 0) {
            continue;
        }

        TokenType type;
        switch (character) {
            case '!':
                type = TokenType::Not;
                break;
            case '&':
                type = TokenType::And;
                break;
            case '^':
                type = TokenType::Xor;
                break;
            case '|':
                type = TokenType::Or;
                break;
            case '(':
                type = TokenType::LeftParen;
                break;
            case ')':
                type = TokenType::RightParen;
                break;
            case '0':
            case '1':
                tokens.push_back(Token{
                    .type = TokenType::Constant,
                    .text = character,
                    .position = position
                });
                continue;
            default:
                if (character >= 'A' && character <= 'Z') {
                    tokens.push_back(Token{
                        .type = TokenType::Variable,
                        .text = character,
                        .position = position
                    });
                    continue;
                }

                if (character >= 'a' && character <= 'z') {
                    throw ParseError(
                        "Variables must be uppercase A-Z",
                        position
                    );
                }

                throw ParseError(
                    "Unexpected character '" + std::string(1, character) + "'",
                    position
                );
        }

        tokens.push_back(Token{
            .type = type,
            .text = character,
            .position = position
        });
    }

    tokens.push_back(Token{
        .type = TokenType::End,
        .text = '\0',
        .position = expression.size()
    });

    return tokens;
}

} // namespace radixlab::boolean
