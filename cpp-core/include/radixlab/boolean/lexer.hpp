#pragma once

#include "radixlab/boolean/token.hpp"

#include <string_view>
#include <vector>

namespace radixlab::boolean {

/**
 * Tokenize a single-line or multi-line Boolean expression.
 *
 * Supported tokens: A-Z, 0, 1, !, &, ^, |, (, ), and whitespace.
 * Every token carries its zero-based source position. A final End token is
 * always appended.
 */
class Lexer {
public:
    [[nodiscard]]
    static std::vector<Token> tokenize(std::string_view expression);
};

} // namespace radixlab::boolean
