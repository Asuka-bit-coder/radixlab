#include "radixlab/boolean/canonical_forms.hpp"

#include "radixlab/boolean/evaluator.hpp"
#include "radixlab/boolean/parser.hpp"

#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace radixlab::boolean {

namespace {

std::string join(
    const std::vector<std::string>& parts,
    std::string_view separator
) {
    std::string result;
    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (index != 0) {
            result += separator;
        }
        result += parts[index];
    }
    return result;
}

std::string mintermTerm(
    const TruthTable& table,
    const std::map<char, bool>& assignment
) {
    std::vector<std::string> literals;
    literals.reserve(table.variables.size());

    for (const char variable : table.variables) {
        const bool value = assignment.at(variable);
        literals.push_back(
            value
                ? std::string(1, variable)
                : "!" + std::string(1, variable)
        );
    }

    return "(" + join(literals, " & ") + ")";
}

std::string maxtermClause(
    const TruthTable& table,
    const std::map<char, bool>& assignment
) {
    std::vector<std::string> literals;
    literals.reserve(table.variables.size());

    for (const char variable : table.variables) {
        const bool value = assignment.at(variable);
        literals.push_back(
            value
                ? "!" + std::string(1, variable)
                : std::string(1, variable)
        );
    }

    return "(" + join(literals, " | ") + ")";
}

void verifyForm(
    const std::string& form,
    const TruthTable& expected
) {
    try {
        const ExprPtr parsed = Parser::parse(form);

        for (const auto& row : expected.rows) {
            if (evaluate(*parsed, row.assignment) != row.result) {
                throw std::logic_error(
                    "Generated canonical form failed truth-table verification"
                );
            }
        }
    } catch (const std::logic_error&) {
        throw;
    } catch (const std::exception& error) {
        throw std::logic_error(
            "Generated canonical form could not be verified: " +
            std::string(error.what())
        );
    }
}

} // namespace


CanonicalForms generateCanonicalForms(const Expr& expression) {
    CanonicalForms result;
    result.table = generateTruthTable(expression);

    const std::size_t variableCount = result.table.variables.size();

    for (const auto& row : result.table.rows) {
        if (row.result) {
            result.minterms.push_back(row.index);
        } else {
            result.maxterms.push_back(row.index);
        }
    }

    if (variableCount == 0) {
        const bool value = result.table.rows.front().result;
        result.sop = value ? "1" : "0";
        result.pos = result.sop;
        verifyForm(result.sop, result.table);
        return result;
    }

    std::vector<std::string> sopTerms;
    sopTerms.reserve(result.minterms.size());

    for (const std::size_t minterm : result.minterms) {
        const auto& row = result.table.rows.at(minterm);
        sopTerms.push_back(mintermTerm(result.table, row.assignment));
    }

    std::vector<std::string> posClauses;
    posClauses.reserve(result.maxterms.size());

    for (const std::size_t maxterm : result.maxterms) {
        const auto& row = result.table.rows.at(maxterm);
        posClauses.push_back(maxtermClause(result.table, row.assignment));
    }

    result.sop = sopTerms.empty() ? "0" : join(sopTerms, " | ");
    result.pos = posClauses.empty() ? "1" : join(posClauses, " & ");

    verifyForm(result.sop, result.table);
    verifyForm(result.pos, result.table);

    return result;
}

} // namespace radixlab::boolean
