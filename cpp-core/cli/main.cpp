#include "radixlab/radix_converter.hpp"

#include <charconv>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::size_t kMaxInputLength = 4096;
constexpr std::size_t kMaxFractionDigits = 4096;

struct Options {
    std::string input;
    int sourceBase = 0;
    int targetBase = 0;
    std::size_t maxFractionDigits = 256;
    bool json = false;
};

struct OptionError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

void printUsage(std::ostream& stream) {
    stream
        << "Usage: radixlab-cli --input NUMERAL --from BASE --to BASE [options]\n"
        << "\n"
        << "Convert an exact positional numeral between bases 2 and 36.\n"
        << "\n"
        << "Options:\n"
        << "  --input VALUE                Numeral to convert (required)\n"
        << "  --from BASE                  Source base, 2-36 (required)\n"
        << "  --to BASE                    Target base, 2-36 (required)\n"
        << "  --max-fraction-digits N      Output limit, 0-4096 (default: 256)\n"
        << "  --json                       Emit structured JSON\n"
        << "  --help                       Show this help\n"
        << "  --version                    Show the version\n"
        << "\n"
        << "Exit codes:\n"
        << "  0  success\n"
        << "  1  internal error\n"
        << "  2  invalid arguments or numeral\n"
        << "  3  input or output resource limit exceeded\n";
}

void printVersion() {
    std::cout << "radixlab-cli 0.1.0\n";
}

std::string jsonEscape(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size() + 8);

    for (const char character : text) {
        const unsigned char byte =
            static_cast<unsigned char>(character);

        switch (character) {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\f':
                escaped += "\\f";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if (byte < 0x20U) {
                    constexpr char hex[] = "0123456789abcdef";
                    escaped += "\\u00";
                    escaped.push_back(hex[(byte >> 4U) & 0x0FU]);
                    escaped.push_back(hex[byte & 0x0FU]);
                } else {
                    escaped.push_back(character);
                }
                break;
        }
    }

    return escaped;
}

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

bool isOption(std::string_view argument, std::string_view name) {
    return argument == name;
}

std::string_view optionValue(
    int& index,
    int argc,
    char* argv[],
    std::string_view option
) {
    if (index + 1 >= argc) {
        throw OptionError(
            "Missing value for " + std::string(option)
        );
    }

    ++index;
    return argv[index];
}

Options parseArguments(int argc, char* argv[]) {
    Options options;
    bool sourceSet = false;
    bool targetSet = false;
    bool inputSet = false;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];

        if (isOption(argument, "--help") || isOption(argument, "-h")) {
            printUsage(std::cout);
            std::exit(0);
        }

        if (isOption(argument, "--version")) {
            printVersion();
            std::exit(0);
        }

        if (isOption(argument, "--json")) {
            options.json = true;
            continue;
        }

        if (isOption(argument, "--input")) {
            if (inputSet) {
                throw OptionError("--input may only be provided once");
            }
            options.input = optionValue(index, argc, argv, argument);
            inputSet = true;
            continue;
        }

        if (isOption(argument, "--from")) {
            if (sourceSet) {
                throw OptionError("--from may only be provided once");
            }
            options.sourceBase = parseInteger<int>(
                optionValue(index, argc, argv, argument),
                argument
            );
            sourceSet = true;
            continue;
        }

        if (isOption(argument, "--to")) {
            if (targetSet) {
                throw OptionError("--to may only be provided once");
            }
            options.targetBase = parseInteger<int>(
                optionValue(index, argc, argv, argument),
                argument
            );
            targetSet = true;
            continue;
        }

        if (isOption(argument, "--max-fraction-digits")) {
            options.maxFractionDigits = parseInteger<std::size_t>(
                optionValue(index, argc, argv, argument),
                argument
            );
            continue;
        }

        throw OptionError(
            "Unknown argument: " + std::string(argument)
        );
    }

    if (!inputSet) {
        throw OptionError("Missing required option: --input");
    }

    if (!sourceSet) {
        throw OptionError("Missing required option: --from");
    }

    if (!targetSet) {
        throw OptionError("Missing required option: --to");
    }

    if (options.sourceBase < 2 || options.sourceBase > 36) {
        throw OptionError("--from must be between 2 and 36");
    }

    if (options.targetBase < 2 || options.targetBase > 36) {
        throw OptionError("--to must be between 2 and 36");
    }

    return options;
}

void emitError(bool json, std::string_view code, std::string_view message) {
    if (json) {
        std::cerr
            << "{\"error\":{\"code\":\"" << jsonEscape(code)
            << "\",\"message\":\"" << jsonEscape(message)
            << "\"}}\n";
    } else {
        std::cerr << "error: " << message << '\n';
    }
}

int run(int argc, char* argv[]) {
    Options options;
    try {
        options = parseArguments(argc, argv);
    } catch (const OptionError& error) {
        emitError(false, "invalid_request", error.what());
        return 2;
    }

    if (options.input.size() > kMaxInputLength) {
        emitError(
            options.json,
            "input_too_large",
            "Input exceeds the 4096-character limit"
        );
        return 3;
    }

    if (options.maxFractionDigits > kMaxFractionDigits) {
        emitError(
            options.json,
            "invalid_limit",
            "Fraction digit limit must not exceed 4096"
        );
        return 2;
    }

    try {
        const radixlab::ConversionResult conversion =
            radixlab::RadixConverter::convert(
                options.input,
                options.sourceBase,
                options.targetBase,
                options.maxFractionDigits
            );

        if (options.json) {
            std::cout
                << "{\"input\":\"" << jsonEscape(options.input)
                << "\",\"sourceBase\":" << options.sourceBase
                << ",\"targetBase\":" << options.targetBase
                << ",\"result\":\"" << jsonEscape(conversion.result)
                << "\",\"exact\":"
                << (conversion.exact ? "true" : "false")
                << ",\"truncated\":"
                << (conversion.truncated ? "true" : "false")
                << ",\"repeating\":"
                << (conversion.repeating ? "true" : "false")
                << ",\"maxFractionDigits\":"
                << options.maxFractionDigits
                << "}\n";
        } else {
            std::cout << conversion.result << '\n';
        }

        return 0;
    } catch (const std::invalid_argument& error) {
        emitError(options.json, "invalid_number", error.what());
        return 2;
    } catch (const std::out_of_range& error) {
        emitError(options.json, "out_of_range", error.what());
        return 2;
    } catch (const std::exception& error) {
        emitError(options.json, "internal_error", error.what());
        return 1;
    }
}

} // namespace


int main(int argc, char* argv[]) {
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
