#include "cli_io.hpp"
#include "encoding_commands.hpp"
#include "radixlab/radix_converter.hpp"

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#ifndef RADIXLAB_VERSION
#define RADIXLAB_VERSION "dev"
#endif

namespace {

using radixlab::cli::OptionError;
using radixlab::cli::parseInteger;
using radixlab::cli::optionValue;

constexpr std::size_t kMaxInputLength = 4096;
constexpr std::size_t kMaxFractionDigits = 4096;

struct Options {
    std::string input;
    int sourceBase = 0;
    int targetBase = 0;
    std::size_t maxFractionDigits = 256;
    bool json = false;
};

void printUsage(std::ostream& stream) {
    stream
        << "Usage:\n"
        << "  radixlab-cli --input NUMERAL --from BASE --to BASE [options]\n"
        << "  radixlab-cli encoding --operation OP [options]\n"
        << "\n"
        << "Convert an exact positional numeral between bases 2 and 36, or\n"
        << "run a digital logic encoding experiment.\n"
        << "\n"
        << "Conversion options:\n"
        << "  --input VALUE                Numeral to convert (required)\n"
        << "  --from BASE                  Source base, 2-36 (required)\n"
        << "  --to BASE                    Target base, 2-36 (required)\n"
        << "  --max-fraction-digits N      Output limit, 0-4096 (default: 256)\n"
        << "  --json                       Emit structured JSON\n"
        << "  --help                       Show this help\n"
        << "  --version                    Show the version\n"
        << "\n"
        << "Encoding operations:\n"
        << "  signed-encode, signed-decode, bcd-encode, bcd-decode,\n"
        << "  gray-encode, gray-decode, parity\n"
        << "  Run 'radixlab-cli encoding --help' for their options.\n"
        << "\n"
        << "Exit codes:\n"
        << "  0  success\n"
        << "  1  internal error\n"
        << "  2  invalid arguments or input\n"
        << "  3  input or output resource limit exceeded\n"
        << "  4  value outside the representable range\n";
}

void printVersion() {
    std::cout << "radixlab-cli " RADIXLAB_VERSION "\n";
}

Options parseArguments(int argc, char* argv[]) {
    Options options;
    bool sourceSet = false;
    bool targetSet = false;
    bool inputSet = false;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];

        if (argument == "--help" || argument == "-h") {
            printUsage(std::cout);
            std::exit(0);
        }

        if (argument == "--version") {
            printVersion();
            std::exit(0);
        }

        if (argument == "--json") {
            options.json = true;
            continue;
        }

        if (argument == "--input") {
            if (inputSet) {
                throw OptionError("--input may only be provided once");
            }
            options.input = optionValue(index, argc, argv, argument);
            inputSet = true;
            continue;
        }

        if (argument == "--from") {
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

        if (argument == "--to") {
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

        if (argument == "--max-fraction-digits") {
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

int runConversion(int argc, char* argv[]) {
    const bool jsonRequested = radixlab::cli::wantsJson(argc, argv);

    Options options;
    try {
        options = parseArguments(argc, argv);
    } catch (const OptionError& error) {
        radixlab::cli::emitError(
            jsonRequested,
            "invalid_request",
            error.what()
        );
        return 2;
    }

    if (options.input.size() > kMaxInputLength) {
        radixlab::cli::emitError(
            options.json,
            "input_too_large",
            "Input exceeds the 4096-character limit"
        );
        return 3;
    }

    if (options.maxFractionDigits > kMaxFractionDigits) {
        radixlab::cli::emitError(
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
                << "{\"input\":"
                << radixlab::cli::jsonString(options.input)
                << ",\"sourceBase\":" << options.sourceBase
                << ",\"targetBase\":" << options.targetBase
                << ",\"result\":"
                << radixlab::cli::jsonString(conversion.result)
                << ",\"exact\":"
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
        radixlab::cli::emitError(
            options.json,
            "invalid_number",
            error.what()
        );
        return 2;
    } catch (const std::out_of_range& error) {
        radixlab::cli::emitError(
            options.json,
            "out_of_range",
            error.what()
        );
        return 2;
    } catch (const std::exception& error) {
        radixlab::cli::emitError(
            options.json,
            "internal_error",
            error.what()
        );
        return 1;
    }
}

} // namespace


int main(int argc, char* argv[]) {
    try {
        if (argc > 1
            && radixlab::cli::isEncodingCommand(argv[1])) {
            return radixlab::cli::runEncodingCommand(argc, argv);
        }

        return runConversion(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
