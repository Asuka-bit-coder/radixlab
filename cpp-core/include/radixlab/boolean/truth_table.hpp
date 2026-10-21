#pragma once

#include "radixlab/boolean/ast.hpp"

#include <cstddef>
#include <map>
#include <vector>

namespace radixlab::boolean {

struct TruthTableRow {
    std::size_t index;
    std::map<char, bool> assignment;
    bool result;
};

struct TruthTable {
    std::vector<char> variables;
    std::vector<TruthTableRow> rows;
};

/**
 * Enumerate the function over all variable assignments.
 *
 * Variables are ordered A-Z. The first variable is the most-significant bit of
 * the row index, so A,B yields row order 00, 01, 10, 11. A constant expression
 * has one row with an empty assignment. At most six variables are allowed.
 */
[[nodiscard]]
TruthTable generateTruthTable(const Expr& expression);

} // namespace radixlab::boolean
