#pragma once

#include "radixlab/boolean/ast.hpp"

#include <optional>
#include <vector>

namespace radixlab::boolean {

struct Counterexample {
    Assignment assignment;
    bool leftResult;
    bool rightResult;
};

struct EquivalenceResult {
    bool equivalent;
    std::vector<char> variables;
    std::optional<Counterexample> counterexample;
};

/**
 * Compare two expressions over the sorted union of their variables. At most
 * six distinct variables are allowed. A non-equivalent result contains the
 * first counterexample in deterministic row order.
 */
[[nodiscard]]
EquivalenceResult checkEquivalent(const Expr& left, const Expr& right);

} // namespace radixlab::boolean
