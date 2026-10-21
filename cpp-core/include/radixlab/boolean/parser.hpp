#pragma once

#include "radixlab/boolean/ast.hpp"

#include <string_view>

namespace radixlab::boolean {

/**
 * Recursive-descent parser for:
 *
 *   expression := or
 *   or         := xor ( "|" xor )*
 *   xor        := and ( "^" and )*
 *   and        := unary ( "&" unary )*
 *   unary      := "!" unary | primary
 *   primary    := VARIABLE | "0" | "1" | "(" expression ")"
 *
 * Precedence: NOT > AND > XOR > OR. Binary operators are left-associative.
 * Implicit operators are not accepted.
 */
class Parser {
public:
    [[nodiscard]]
    static ExprPtr parse(std::string_view expression);
};

} // namespace radixlab::boolean
