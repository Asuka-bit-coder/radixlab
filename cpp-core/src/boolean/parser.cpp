#include "radixlab/boolean/parser.hpp"

#include "radixlab/boolean/boolean_error.hpp"
#include "radixlab/boolean/lexer.hpp"
#include "radixlab/boolean/limits.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace radixlab::boolean {

namespace {

class ParserImpl {
public:
    explicit ParserImpl(std::vector<Token> tokens)
        : tokens_(std::move(tokens)) {
    }

    ExprPtr parseAll() {
        if (check(TokenType::End)) {
            throw ParseError("Expression cannot be empty", 0);
        }

        ExprPtr expression = parseOr();

        if (!check(TokenType::End)) {
            throw ParseError(
                "Unexpected token after complete expression",
                peek().position
            );
        }

        return expression;
    }

private:
    std::vector<Token> tokens_;
    std::size_t current_ = 0;
    std::size_t nodeCount_ = 0;
    std::size_t nesting_ = 0;

    const Token& peek() const {
        return tokens_[current_];
    }

    const Token& previous() const {
        return tokens_[current_ - 1];
    }

    bool check(TokenType type) const {
        return peek().type == type;
    }

    bool match(TokenType type) {
        if (!check(type)) {
            return false;
        }

        ++current_;
        return true;
    }

    void countNode() {
        ++nodeCount_;
        if (nodeCount_ > kMaxAstNodes) {
            throw BooleanLimitError(
                "Expression contains too many AST nodes"
            );
        }
    }

    void enterNesting() {
        ++nesting_;
        if (nesting_ > kMaxNestingDepth) {
            throw BooleanLimitError(
                "Expression nesting exceeds the 64-level limit"
            );
        }
    }

    void leaveNesting() {
        --nesting_;
    }

    ExprPtr makeVariable(char name) {
        countNode();
        return std::make_unique<VariableExpr>(name);
    }

    ExprPtr makeConstant(bool value) {
        countNode();
        return std::make_unique<ConstantExpr>(value);
    }

    ExprPtr makeNot(ExprPtr operand) {
        countNode();
        return std::make_unique<NotExpr>(std::move(operand));
    }

    ExprPtr makeBinary(
        BinaryOperator op,
        ExprPtr left,
        ExprPtr right
    ) {
        countNode();
        return std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );
    }

    ExprPtr parseOr() {
        ExprPtr expression = parseXor();

        while (match(TokenType::Or)) {
            ExprPtr right = parseXor();
            expression = makeBinary(
                BinaryOperator::Or,
                std::move(expression),
                std::move(right)
            );
        }

        return expression;
    }

    ExprPtr parseXor() {
        ExprPtr expression = parseAnd();

        while (match(TokenType::Xor)) {
            ExprPtr right = parseAnd();
            expression = makeBinary(
                BinaryOperator::Xor,
                std::move(expression),
                std::move(right)
            );
        }

        return expression;
    }

    ExprPtr parseAnd() {
        ExprPtr expression = parseUnary();

        while (match(TokenType::And)) {
            ExprPtr right = parseUnary();
            expression = makeBinary(
                BinaryOperator::And,
                std::move(expression),
                std::move(right)
            );
        }

        return expression;
    }

    ExprPtr parseUnary() {
        if (match(TokenType::Not)) {
            enterNesting();
            ExprPtr operand = parseUnary();
            leaveNesting();
            return makeNot(std::move(operand));
        }

        return parsePrimary();
    }

    ExprPtr parsePrimary() {
        if (match(TokenType::Variable)) {
            return makeVariable(previous().text);
        }

        if (match(TokenType::Constant)) {
            return makeConstant(previous().text == '1');
        }

        if (match(TokenType::LeftParen)) {
            const std::size_t opening = previous().position;
            enterNesting();
            ExprPtr expression = parseOr();
            leaveNesting();

            if (!match(TokenType::RightParen)) {
                throw ParseError(
                    "Expected ')' to close group opened at column " +
                        std::to_string(opening + 1U),
                    peek().position
                );
            }

            return expression;
        }

        throw ParseError(
            "Expected a variable, 0, 1, '!', or '('",
            peek().position
        );
    }
};

} // namespace


ExprPtr Parser::parse(std::string_view expression) {
    ParserImpl parser(Lexer::tokenize(expression));
    return parser.parseAll();
}

} // namespace radixlab::boolean
