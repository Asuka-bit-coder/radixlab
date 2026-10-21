#pragma once

#include "radixlab/boolean/ast.hpp"

namespace radixlab::boolean {

/**
 * Evaluate an expression. AND and OR short-circuit, so only variables on an
 * evaluated path require an assignment entry. A missing required variable
 * throws EvaluationError.
 */
[[nodiscard]]
bool evaluate(const Expr& expression, const Assignment& assignment);

} // namespace radixlab::boolean
