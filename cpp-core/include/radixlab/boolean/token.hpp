#pragma once

#include <cstddef>
#include <string_view>

namespace radixlab::boolean {

enum class TokenType {
    Variable,
    Constant,
    Not,
    And,
    Xor,
    Or,
    LeftParen,
    RightParen,
    End
};

struct Token {
    TokenType type;
    char text;
    std::size_t position;
};

[[nodiscard]]
std::string_view tokenTypeName(TokenType type) noexcept;

} // namespace radixlab::boolean
