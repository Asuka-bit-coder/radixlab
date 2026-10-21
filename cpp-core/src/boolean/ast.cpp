#include "radixlab/boolean/ast.hpp"

#include "radixlab/boolean/boolean_error.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace radixlab::boolean {

namespace {

bool evaluatedOperand(const Expr& expression, const Assignment& assignment) {
    return expression.evaluate(assignment);
}

std::string booleanJson(bool value) {
    return value ? "true" : "false";
}

} // namespace


VariableExpr::VariableExpr(char name)
    : name(name) {
    if (name < 'A' || name > 'Z') {
        throw std::invalid_argument("Variable name must be A-Z");
    }
}


bool VariableExpr::evaluate(const Assignment& assignment) const {
    const auto found = assignment.find(name);

    if (found == assignment.end()) {
        throw EvaluationError(
            "Missing value for variable " + std::string(1, name)
        );
    }

    return found->second;
}


void VariableExpr::collectVariables(std::set<char>& variables) const {
    variables.insert(name);
}


std::string VariableExpr::toString() const {
    return std::string(1, name);
}


std::string VariableExpr::toJson() const {
    return "{\"type\":\"variable\",\"name\":\"" +
        std::string(1, name) + "\"}";
}


ConstantExpr::ConstantExpr(bool value)
    : value(value) {
}


bool ConstantExpr::evaluate(const Assignment&) const {
    return value;
}


void ConstantExpr::collectVariables(std::set<char>&) const {
}


std::string ConstantExpr::toString() const {
    return value ? "1" : "0";
}


std::string ConstantExpr::toJson() const {
    return "{\"type\":\"constant\",\"value\":" + booleanJson(value) + "}";
}


NotExpr::NotExpr(ExprPtr operand)
    : operand(std::move(operand)) {
    if (!this->operand) {
        throw std::invalid_argument("NOT requires an operand");
    }
}


bool NotExpr::evaluate(const Assignment& assignment) const {
    return !evaluatedOperand(*operand, assignment);
}


void NotExpr::collectVariables(std::set<char>& variables) const {
    operand->collectVariables(variables);
}


std::string NotExpr::toString() const {
    return "!" + operand->toString();
}


std::string NotExpr::toJson() const {
    return "{\"type\":\"not\",\"operand\":" + operand->toJson() + "}";
}


char binaryOperatorSymbol(BinaryOperator op) noexcept {
    switch (op) {
        case BinaryOperator::And:
            return '&';
        case BinaryOperator::Or:
            return '|';
        case BinaryOperator::Xor:
            return '^';
    }

    return '?';
}


std::string_view binaryOperatorName(BinaryOperator op) noexcept {
    switch (op) {
        case BinaryOperator::And:
            return "and";
        case BinaryOperator::Or:
            return "or";
        case BinaryOperator::Xor:
            return "xor";
    }

    return "unknown";
}


BinaryExpr::BinaryExpr(BinaryOperator op, ExprPtr left, ExprPtr right)
    : op(op),
      left(std::move(left)),
      right(std::move(right)) {
    if (!this->left || !this->right) {
        throw std::invalid_argument("Binary operator requires two operands");
    }
}


bool BinaryExpr::evaluate(const Assignment& assignment) const {
    const bool leftValue = evaluatedOperand(*left, assignment);

    switch (op) {
        case BinaryOperator::And:
            if (!leftValue) {
                return false;
            }
            return evaluatedOperand(*right, assignment);
        case BinaryOperator::Or:
            if (leftValue) {
                return true;
            }
            return evaluatedOperand(*right, assignment);
        case BinaryOperator::Xor:
            return leftValue != evaluatedOperand(*right, assignment);
    }

    throw std::logic_error("Unknown binary operator");
}


void BinaryExpr::collectVariables(std::set<char>& variables) const {
    left->collectVariables(variables);
    right->collectVariables(variables);
}


std::string BinaryExpr::toString() const {
    return "(" + left->toString() + " " +
        std::string(1, binaryOperatorSymbol(op)) + " " +
        right->toString() + ")";
}


std::string BinaryExpr::toJson() const {
    return "{\"type\":\"" + std::string(binaryOperatorName(op)) +
        "\",\"left\":" + left->toJson() +
        ",\"right\":" + right->toJson() + "}";
}

} // namespace radixlab::boolean
