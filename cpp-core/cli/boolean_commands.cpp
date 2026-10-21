#include "boolean_commands.hpp"

#include "cli_io.hpp"
#include "radixlab/boolean/ast.hpp"
#include "radixlab/boolean/boolean_error.hpp"
#include "radixlab/boolean/canonical_forms.hpp"
#include "radixlab/boolean/equivalence.hpp"
#include "radixlab/boolean/evaluator.hpp"
#include "radixlab/boolean/parser.hpp"
#include "radixlab/boolean/truth_table.hpp"

#include <cctype>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace radixlab {
namespace cli {

namespace {

constexpr std::size_t kMaxRequestExpressionLength = 1024;

struct BooleanOptions {
    std::string operation;
    std::map<std::string, std::string> values;
    bool json = false;
};

std::string normalizeOperation(std::string_view raw) {
    std::string normalized;
    normalized.reserve(raw.size());

    for (const char character : raw) {
        const unsigned char byte = static_cast<unsigned char>(character);
        normalized.push_back(character == '_'
            ? '-'
            : static_cast<char>(std::tolower(byte)));
    }

    return normalized;
}

std::string trim(std::string_view text) {
    std::size_t begin = 0;
    std::size_t end = text.size();

    while (begin < end
        && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    while (end > begin
        && std::isspace(static_cast<unsigned char>(text[end - 1U])) != 0) {
        --end;
    }

    return std::string(text.substr(begin, end - begin));
}

BooleanOptions parseOptions(int argc, char* argv[]) {
    BooleanOptions options;

    if (argc < 3) {
        throw OptionError("Missing boolean operation");
    }

    options.operation = normalizeOperation(argv[2]);

    for (int index = 3; index < argc; ++index) {
        const std::string_view argument = argv[index];

        if (argument == "--help" || argument == "-h") {
            printBooleanHelp(std::cout);
            std::exit(0);
        }

        if (argument == "--json") {
            options.json = true;
            continue;
        }

        if (argument.size() < 3 || argument.substr(0, 2) != "--") {
            throw OptionError(
                "Unexpected positional argument: " + std::string(argument)
            );
        }

        const std::string name(argument.substr(2));
        if (options.values.contains(name)) {
            throw OptionError("--" + name + " may only be provided once");
        }

        options.values.emplace(
            name,
            std::string(optionValue(index, argc, argv, argument))
        );
    }

    if (options.operation == "--help") {
        printBooleanHelp(std::cout);
        std::exit(0);
    }

    return options;
}

void rejectUnknown(
    const BooleanOptions& options,
    std::initializer_list<std::string_view> allowed
) {
    for (const auto& [name, value] : options.values) {
        (void)value;
        bool known = false;
        for (const std::string_view candidate : allowed) {
            if (name == candidate) {
                known = true;
                break;
            }
        }

        if (!known) {
            throw OptionError(
                "Unknown option for boolean " + options.operation +
                ": --" + name
            );
        }
    }
}

const std::string& required(
    const BooleanOptions& options,
    std::string_view name
) {
    const auto found = options.values.find(std::string(name));
    if (found == options.values.end()) {
        throw OptionError(
            "Missing required option: --" + std::string(name)
        );
    }
    return found->second;
}

std::string checkExpressionLength(std::string expression) {
    if (expression.size() > kMaxRequestExpressionLength) {
        throw boolean::BooleanLimitError(
            "Expression exceeds the 1024-character request limit"
        );
    }
    return expression;
}

boolean::ExprPtr parseExpression(const std::string& expression) {
    return boolean::Parser::parse(checkExpressionLength(expression));
}

boolean::Assignment parseAssignment(std::string_view text) {
    boolean::Assignment assignment;
    const std::string input = trim(text);

    if (input.empty()) {
        return assignment;
    }

    std::size_t begin = 0;
    while (begin <= input.size()) {
        const std::size_t comma = input.find(',', begin);
        const std::string token = trim(
            std::string_view(input).substr(
                begin,
                comma == std::string::npos
                    ? std::string::npos
                    : comma - begin
            )
        );

        const std::size_t equals = token.find('=');
        if (equals == std::string::npos) {
            throw OptionError(
                "Assignment entries must use NAME=0 or NAME=1"
            );
        }

        const std::string name = trim(
            std::string_view(token).substr(0, equals)
        );
        const std::string value = trim(
            std::string_view(token).substr(equals + 1U)
        );

        if (name.size() != 1U || name[0] < 'A' || name[0] > 'Z') {
            throw OptionError(
                "Assignment names must be single uppercase letters A-Z"
            );
        }

        if (value != "0" && value != "1") {
            throw OptionError("Assignment values must be 0 or 1");
        }

        if (assignment.contains(name[0])) {
            throw OptionError(
                "Duplicate assignment for variable " + name
            );
        }

        assignment.emplace(name[0], value == "1");

        if (comma == std::string::npos) {
            break;
        }
        begin = comma + 1U;
    }

    return assignment;
}

std::string variablesJson(const std::vector<char>& variables) {
    std::string json = "[";
    for (std::size_t index = 0; index < variables.size(); ++index) {
        if (index != 0) {
            json += ",";
        }
        json += jsonString(std::string(1, variables[index]));
    }
    json += "]";
    return json;
}

std::string assignmentJson(const boolean::Assignment& assignment) {
    std::string json = "{";
    bool first = true;

    for (const auto& [name, value] : assignment) {
        if (!first) {
            json += ",";
        }
        first = false;
        json += jsonString(std::string(1, name));
        json += ":";
        json += value ? "true" : "false";
    }

    json += "}";
    return json;
}

std::vector<char> variablesOf(const boolean::Expr& expression) {
    std::set<char> variables;
    expression.collectVariables(variables);
    return std::vector<char>(variables.begin(), variables.end());
}

void emitBooleanError(
    bool json,
    std::string_view code,
    std::string_view message,
    std::optional<std::size_t> position = std::nullopt
) {
    if (json) {
        std::cerr
            << "{\"error\":{\"code\":" << jsonString(code)
            << ",\"message\":" << jsonString(message);
        if (position.has_value()) {
            std::cerr
                << ",\"position\":" << *position
                << ",\"column\":" << (*position + 1U);
        }
        std::cerr << "}}\n";
        return;
    }

    std::cerr << "error: " << message;
    if (position.has_value()) {
        std::cerr << " at column " << (*position + 1U);
    }
    std::cerr << '\n';
}

void runEvaluate(const BooleanOptions& options) {
    rejectUnknown(options, {"expression", "assignment"});
    const auto expression = parseExpression(required(options, "expression"));

    const auto found = options.values.find("assignment");
    const boolean::Assignment assignment = found == options.values.end()
        ? boolean::Assignment{}
        : parseAssignment(found->second);

    const bool result = boolean::evaluate(*expression, assignment);

    if (options.json) {
        std::cout
            << "{\"expression\":"
            << jsonString(required(options, "expression"))
            << ",\"normalized\":" << jsonString(expression->toString())
            << ",\"variables\":"
            << variablesJson(variablesOf(*expression))
            << ",\"assignment\":" << assignmentJson(assignment)
            << ",\"result\":" << (result ? "true" : "false")
            << ",\"ast\":" << expression->toJson()
            << "}\n";
    } else {
        std::cout << "result=" << (result ? "true" : "false") << '\n';
    }
}

void runTruthTable(const BooleanOptions& options) {
    rejectUnknown(options, {"expression"});
    const auto expression = parseExpression(required(options, "expression"));
    const auto table = boolean::generateTruthTable(*expression);

    if (options.json) {
        std::cout
            << "{\"expression\":"
            << jsonString(required(options, "expression"))
            << ",\"normalized\":" << jsonString(expression->toString())
            << ",\"variables\":" << variablesJson(table.variables)
            << ",\"rowCount\":" << table.rows.size()
            << ",\"rows\":[";

        for (std::size_t index = 0; index < table.rows.size(); ++index) {
            if (index != 0) {
                std::cout << ",";
            }
            const auto& row = table.rows[index];
            std::cout
                << "{\"index\":" << row.index
                << ",\"assignment\":" << assignmentJson(row.assignment)
                << ",\"result\":" << (row.result ? "true" : "false")
                << "}";
        }

        std::cout
            << "],\"ast\":" << expression->toJson()
            << "}\n";
    } else {
        for (const char variable : table.variables) {
            std::cout << variable << '\t';
        }
        std::cout << "Result\n";

        for (const auto& row : table.rows) {
            for (const char variable : table.variables) {
                std::cout << (row.assignment.at(variable) ? '1' : '0') << '\t';
            }
            std::cout << (row.result ? '1' : '0') << '\n';
        }
    }
}

void printIndexList(const std::vector<std::size_t>& values) {
    std::cout << "[";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            std::cout << ",";
        }
        std::cout << values[index];
    }
    std::cout << "]";
}

void runCanonical(const BooleanOptions& options) {
    rejectUnknown(options, {"expression"});
    const auto expression = parseExpression(required(options, "expression"));
    const auto forms = boolean::generateCanonicalForms(*expression);

    if (options.json) {
        std::cout
            << "{\"expression\":"
            << jsonString(required(options, "expression"))
            << ",\"normalized\":" << jsonString(expression->toString())
            << ",\"variables\":" << variablesJson(forms.table.variables)
            << ",\"rowCount\":" << forms.table.rows.size()
            << ",\"minterms\":";
        printIndexList(forms.minterms);
        std::cout << ",\"maxterms\":";
        printIndexList(forms.maxterms);
        std::cout
            << ",\"sop\":" << jsonString(forms.sop)
            << ",\"pos\":" << jsonString(forms.pos)
            << ",\"ast\":" << expression->toJson()
            << "}\n";
    } else {
        std::cout << "SOP: " << forms.sop << '\n';
        std::cout << "POS: " << forms.pos << '\n';
    }
}

void runEquivalent(const BooleanOptions& options) {
    rejectUnknown(options, {"left", "right"});
    const auto left = parseExpression(required(options, "left"));
    const auto right = parseExpression(required(options, "right"));
    const auto result = boolean::checkEquivalent(*left, *right);

    if (options.json) {
        std::cout
            << "{\"leftExpression\":"
            << jsonString(required(options, "left"))
            << ",\"rightExpression\":"
            << jsonString(required(options, "right"))
            << ",\"leftNormalized\":" << jsonString(left->toString())
            << ",\"rightNormalized\":" << jsonString(right->toString())
            << ",\"variables\":" << variablesJson(result.variables)
            << ",\"equivalent\":"
            << (result.equivalent ? "true" : "false")
            << ",\"leftAst\":" << left->toJson()
            << ",\"rightAst\":" << right->toJson()
            << ",\"counterexample\":";

        if (result.counterexample.has_value()) {
            const auto& counterexample = *result.counterexample;
            std::cout
                << "{\"assignment\":"
                << assignmentJson(counterexample.assignment)
                << ",\"left\":"
                << (counterexample.leftResult ? "true" : "false")
                << ",\"right\":"
                << (counterexample.rightResult ? "true" : "false")
                << "}";
        } else {
            std::cout << "null";
        }

        std::cout << "}\n";
    } else if (result.equivalent) {
        std::cout << "equivalent=true\n";
    } else {
        std::cout << "equivalent=false counterexample=";
        const auto& counterexample = *result.counterexample;
        bool first = true;
        for (const auto& [name, value] : counterexample.assignment) {
            if (!first) {
                std::cout << ",";
            }
            first = false;
            std::cout << name << "=" << (value ? '1' : '0');
        }
        std::cout << '\n';
    }
}

int dispatch(const BooleanOptions& options) {
    if (options.operation == "evaluate") {
        runEvaluate(options);
    } else if (options.operation == "truth-table") {
        runTruthTable(options);
    } else if (options.operation == "canonical-forms") {
        runCanonical(options);
    } else if (options.operation == "equivalent") {
        runEquivalent(options);
    } else {
        throw OptionError(
            "Unknown boolean operation: " + options.operation
        );
    }

    return 0;
}

} // namespace


void printBooleanHelp(std::ostream& stream) {
    stream
        << "Usage:\n"
        << "  radixlab-cli boolean evaluate --expression EXPR [--assignment LIST]\n"
        << "  radixlab-cli boolean truth-table --expression EXPR\n"
        << "  radixlab-cli boolean canonical-forms --expression EXPR\n"
        << "  radixlab-cli boolean equivalent --left EXPR --right EXPR\n"
        << "\n"
        << "Operators: ! (NOT), & (AND), ^ (XOR), | (OR), parentheses.\n"
        << "Precedence: NOT > AND > XOR > OR. Binary operators are left-associative.\n"
        << "Variables: A-Z. Constants: 0 and 1. Maximum 6 variables for tables.\n"
        << "Assignment syntax: --assignment \"A=1,B=0\" (omit unused variables).\n"
        << "Add --json for structured output.\n"
        << "\n"
        << "Examples:\n"
        << "  radixlab-cli boolean evaluate --expression \"A & (!B | C)\" \\\n"
        << "    --assignment \"A=1,B=0,C=0\" --json\n"
        << "  radixlab-cli boolean truth-table --expression \"A & B\" --json\n"
        << "  radixlab-cli boolean equivalent --left \"!(A & B)\" \\\n"
        << "    --right \"!A | !B\" --json\n"
        << "\n"
        << "Exit codes:\n"
        << "  0  success\n"
        << "  1  internal error\n"
        << "  2  invalid expression, assignment, or arguments\n"
        << "  3  expression or variable limit exceeded\n";
}


bool isBooleanCommand(std::string_view firstArgument) {
    return firstArgument == "boolean";
}


int runBooleanCommand(int argc, char* argv[]) {
    const bool jsonRequested = wantsJson(argc, argv);

    try {
        const BooleanOptions options = parseOptions(argc, argv);
        return dispatch(options);
    } catch (const boolean::ParseError& error) {
        emitBooleanError(
            jsonRequested,
            "parse_error",
            error.what(),
            error.position()
        );
        return 2;
    } catch (const boolean::BooleanLimitError& error) {
        emitBooleanError(jsonRequested, "limit", error.what());
        return 3;
    } catch (const boolean::EvaluationError& error) {
        emitBooleanError(jsonRequested, "evaluation_error", error.what());
        return 2;
    } catch (const OptionError& error) {
        emitBooleanError(jsonRequested, "invalid_request", error.what());
        return 2;
    } catch (const std::invalid_argument& error) {
        emitBooleanError(jsonRequested, "invalid_input", error.what());
        return 2;
    } catch (const std::exception& error) {
        emitBooleanError(jsonRequested, "internal_error", error.what());
        return 1;
    }
}

} // namespace cli
} // namespace radixlab
