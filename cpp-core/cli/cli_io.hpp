#pragma once

#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>

namespace radixlab {
namespace cli {

struct OptionError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

[[nodiscard]]
std::string jsonEscape(std::string_view text);

[[nodiscard]]
std::string jsonString(std::string_view text);

void emitError(bool json, std::string_view code, std::string_view message);

[[nodiscard]]
bool wantsJson(int argc, char* argv[]);

template <typename Integer>
Integer parseInteger(std::string_view text, std::string_view option) {
    Integer value{};
    const char* const begin = text.data();
    const char* const end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, value);

    if (result.ec != std::errc{} || result.ptr != end) {
        throw OptionError(
            "Invalid integer for " + std::string(option) + ": " +
            std::string(text)
        );
    }

    return value;
}

[[nodiscard]]
std::string_view optionValue(
    int& index,
    int argc,
    char* argv[],
    std::string_view option
);

} // namespace cli
} // namespace radixlab
