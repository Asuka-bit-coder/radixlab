#pragma once

#include "radixlab/boolean/ast.hpp"
#include "radixlab/boolean/truth_table.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace radixlab::boolean {

/**
 * Canonical SOP and POS derived from a truth table.
 *
 * Minterm/maxterm indices use the same variable order as TruthTable, with the
 * first variable as the most-significant bit. These are canonical expansions,
 * not minimized expressions. Constant functions use the conventional empty
 * sum/product notation for the absent side.
 */
struct CanonicalForms {
    TruthTable table;
    std::vector<std::size_t> minterms;
    std::vector<std::size_t> maxterms;
    std::string sop;
    std::string pos;
};

[[nodiscard]]
CanonicalForms generateCanonicalForms(const Expr& expression);

} // namespace radixlab::boolean
