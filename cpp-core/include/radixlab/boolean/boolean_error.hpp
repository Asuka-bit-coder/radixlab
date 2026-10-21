#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace radixlab::boolean {

class BooleanError : public std::runtime_error {
public:
    explicit BooleanError(
        std::string message,
        std::size_t position = std::string::npos
    )
        : std::runtime_error(std::move(message)),
          position_(position) {
    }

    [[nodiscard]]
    std::size_t position() const noexcept {
        return position_;
    }

    [[nodiscard]]
    bool hasPosition() const noexcept {
        return position_ != std::string::npos;
    }

private:
    std::size_t position_;
};

class ParseError final : public BooleanError {
public:
    ParseError(std::string message, std::size_t position)
        : BooleanError(std::move(message), position) {
    }
};

class EvaluationError final : public BooleanError {
public:
    explicit EvaluationError(std::string message)
        : BooleanError(std::move(message)) {
    }
};

class BooleanLimitError final : public BooleanError {
public:
    explicit BooleanLimitError(std::string message)
        : BooleanError(std::move(message)) {
    }
};

} // namespace radixlab::boolean
