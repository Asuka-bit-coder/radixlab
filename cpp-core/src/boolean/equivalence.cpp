#include "radixlab/boolean/equivalence.hpp"

#include "radixlab/boolean/boolean_error.hpp"
#include "radixlab/boolean/evaluator.hpp"
#include "radixlab/boolean/limits.hpp"

#include <cstddef>
#include <set>
#include <utility>

namespace radixlab::boolean {

EquivalenceResult checkEquivalent(const Expr& left, const Expr& right) {
    std::set<char> variableSet;
    left.collectVariables(variableSet);
    right.collectVariables(variableSet);

    if (variableSet.size() > kMaxTruthTableVariables) {
        throw BooleanLimitError(
            "Equivalence checks are limited to 6 distinct variables"
        );
    }

    EquivalenceResult result;
    result.variables.assign(variableSet.begin(), variableSet.end());

    const std::size_t variableCount = result.variables.size();
    const std::size_t rowCount =
        variableCount == 0 ? 1U : (std::size_t{1} << variableCount);

    for (std::size_t index = 0; index < rowCount; ++index) {
        Assignment assignment;

        for (std::size_t position = 0; position < variableCount; ++position) {
            const std::size_t shift = variableCount - position - 1U;
            assignment.emplace(
                result.variables[position],
                ((index >> shift) & 1U) != 0U
            );
        }

        const bool leftResult = evaluate(left, assignment);
        const bool rightResult = evaluate(right, assignment);

        if (leftResult != rightResult) {
            result.equivalent = false;
            result.counterexample = Counterexample{
                .assignment = std::move(assignment),
                .leftResult = leftResult,
                .rightResult = rightResult
            };
            return result;
        }
    }

    result.equivalent = true;
    return result;
}

} // namespace radixlab::boolean
