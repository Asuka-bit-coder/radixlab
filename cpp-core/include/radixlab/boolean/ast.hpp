#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>

namespace radixlab::boolean {

using Assignment = std::map<char, bool>;

class Expr {
public:
    virtual ~Expr() = default;

    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;

    [[nodiscard]]
    virtual bool evaluate(const Assignment& assignment) const = 0;

    virtual void collectVariables(std::set<char>& variables) const = 0;

    // Fully parenthesized display form for teaching and debugging.
    [[nodiscard]]
    virtual std::string toString() const = 0;

    // Structured JSON fragment used by the CLI and AST Explorer.
    [[nodiscard]]
    virtual std::string toJson() const = 0;

protected:
    Expr() = default;
};

using ExprPtr = std::unique_ptr<Expr>;

struct VariableExpr final : Expr {
    explicit VariableExpr(char name);

    [[nodiscard]]
    bool evaluate(const Assignment& assignment) const override;

    void collectVariables(std::set<char>& variables) const override;

    [[nodiscard]]
    std::string toString() const override;

    [[nodiscard]]
    std::string toJson() const override;

    char name;
};

struct ConstantExpr final : Expr {
    explicit ConstantExpr(bool value);

    [[nodiscard]]
    bool evaluate(const Assignment& assignment) const override;

    void collectVariables(std::set<char>& variables) const override;

    [[nodiscard]]
    std::string toString() const override;

    [[nodiscard]]
    std::string toJson() const override;

    bool value;
};

struct NotExpr final : Expr {
    explicit NotExpr(ExprPtr operand);

    [[nodiscard]]
    bool evaluate(const Assignment& assignment) const override;

    void collectVariables(std::set<char>& variables) const override;

    [[nodiscard]]
    std::string toString() const override;

    [[nodiscard]]
    std::string toJson() const override;

    ExprPtr operand;
};

enum class BinaryOperator {
    And,
    Or,
    Xor
};

[[nodiscard]]
char binaryOperatorSymbol(BinaryOperator op) noexcept;

[[nodiscard]]
std::string_view binaryOperatorName(BinaryOperator op) noexcept;

struct BinaryExpr final : Expr {
    BinaryExpr(BinaryOperator op, ExprPtr left, ExprPtr right);

    [[nodiscard]]
    bool evaluate(const Assignment& assignment) const override;

    void collectVariables(std::set<char>& variables) const override;

    [[nodiscard]]
    std::string toString() const override;

    [[nodiscard]]
    std::string toJson() const override;

    BinaryOperator op;
    ExprPtr left;
    ExprPtr right;
};

} // namespace radixlab::boolean
