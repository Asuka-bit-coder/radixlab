#pragma once

#include "radixlab/boolean/ast.hpp"

namespace radixlab::boolean {

/**
 * Evaluate an expression. Every variable used by the expression must have an
 * entry in assignment; otherwise EvaluationError is thrown.
 */
[[nodiscard]]
bool evaluate(const Expr& expression, const Assignment& assignment);

} // namespace radixlab::boolean
