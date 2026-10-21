#include "radixlab/boolean/evaluator.hpp"

namespace radixlab::boolean {

bool evaluate(const Expr& expression, const Assignment& assignment) {
    return expression.evaluate(assignment);
}

} // namespace radixlab::boolean
