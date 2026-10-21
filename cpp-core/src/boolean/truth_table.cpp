#include "radixlab/boolean/truth_table.hpp"

#include "radixlab/boolean/boolean_error.hpp"
#include "radixlab/boolean/evaluator.hpp"
#include "radixlab/boolean/limits.hpp"

#include <set>
#include <utility>

namespace radixlab::boolean {

TruthTable generateTruthTable(const Expr& expression) {
    std::set<char> variableSet;
    expression.collectVariables(variableSet);

    if (variableSet.size() > kMaxTruthTableVariables) {
        throw BooleanLimitError(
            "Truth tables are limited to 6 distinct variables"
        );
    }

    TruthTable table;
    table.variables.assign(variableSet.begin(), variableSet.end());

    const std::size_t variableCount = table.variables.size();
    const std::size_t rowCount =
        variableCount == 0 ? 1U : (std::size_t{1} << variableCount);
    table.rows.reserve(rowCount);

    for (std::size_t index = 0; index < rowCount; ++index) {
        TruthTableRow row;
        row.index = index;

        for (std::size_t position = 0; position < variableCount; ++position) {
            const std::size_t shift = variableCount - position - 1U;
            const bool value = ((index >> shift) & 1U) != 0U;
            row.assignment.emplace(table.variables[position], value);
        }

        row.result = evaluate(expression, row.assignment);
        table.rows.push_back(std::move(row));
    }

    return table;
}

} // namespace radixlab::boolean
