#include "encoding_commands.hpp"

#include "cli_io.hpp"
#include "radixlab/big_int.hpp"
#include "radixlab/encoding/bcd.hpp"
#include "radixlab/encoding/bit_vector.hpp"
#include "radixlab/encoding/encoding_error.hpp"
#include "radixlab/encoding/gray_code.hpp"
#include "radixlab/encoding/parity.hpp"
#include "radixlab/encoding/signed_encoding.hpp"

#include <cctype>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace radixlab {
namespace cli {

namespace {

constexpr std::size_t kMaxBitString = 64;
constexpr std::size_t kMaxDecimalValueLength = 64;
constexpr std::size_t kMaxBcdDigits = 1024;
constexpr std::size_t kMaxBcdBits = 4096;
constexpr std::size_t kMaxBcdInputChars = kMaxBcdBits * 2U;

struct LimitError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct EncodingOptions {
    std::string operation;
    std::map<std::string, std::string> values;
    bool json = false;
    bool negativeZero = false;
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

EncodingOptions parseOptions(int argc, char* argv[]) {
    EncodingOptions options;

    for (int index = 2; index < argc; ++index) {
        const std::string_view argument = argv[index];

        if (argument == "--help" || argument == "-h") {
            printEncodingHelp(std::cout);
            std::exit(0);
        }

        if (argument == "--json") {
            options.json = true;
            continue;
        }

        if (argument == "--negative-zero") {
            options.negativeZero = true;
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

    if (!options.values.contains("operation")) {
        throw OptionError("Missing required option: --operation");
    }

    options.operation = normalizeOperation(
        options.values.at("operation")
    );

    if (options.negativeZero && options.operation != "signed-encode") {
        throw OptionError(
            "--negative-zero is only valid for signed-encode"
        );
    }

    return options;
}

void rejectUnknown(
    const EncodingOptions& options,
    std::initializer_list<std::string_view> allowed
) {
    for (const auto& [name, value] : options.values) {
        (void)value;
        if (name == "operation") {
            continue;
        }

        bool known = false;
        for (const std::string_view candidate : allowed) {
            if (name == candidate) {
                known = true;
                break;
            }
        }

        if (!known) {
            throw OptionError(
                "Unknown option for " + options.operation + ": --" + name
            );
        }
    }
}

const std::string& required(
    const EncodingOptions& options,
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

std::size_t parseWidth(const EncodingOptions& options) {
    const int width = parseInteger<int>(
        required(options, "width"),
        "--width"
    );

    if (width < 1 || width > 64) {
        throw OptionError("--width must be between 1 and 64");
    }

    return static_cast<std::size_t>(width);
}

BigInt parseDecimal(
    std::string_view text,
    std::string_view field,
    bool allowNegative
) {
    if (text.empty()) {
        throw OptionError(
            std::string(field) + " cannot be empty"
        );
    }

    if (text.size() > kMaxDecimalValueLength) {
        throw LimitError(
            std::string(field) + " exceeds the 64-character limit"
        );
    }

    bool negative = false;
    std::size_t position = 0;

    if (text[0] == '-' || text[0] == '+') {
        negative = text[0] == '-';
        position = 1;
    }

    if (!allowNegative && negative) {
        throw OptionError(std::string(field) + " cannot be negative");
    }

    if (position == text.size()) {
        throw OptionError(
            std::string(field) + " must contain at least one digit"
        );
    }

    BigInt value = 0;
    for (; position < text.size(); ++position) {
        const char character = text[position];
        if (character < '0' || character > '9') {
            throw OptionError(
                std::string(field) + " must be a decimal integer"
            );
        }
        value = value * 10 + (character - '0');
    }

    return negative ? -value : value;
}

encoding::BitVector parseBitVector(
    std::string_view bits,
    std::string_view field
) {
    if (bits.size() > kMaxBitString) {
        throw OptionError(
            std::string(field) + " exceeds the 64-bit limit"
        );
    }

    return encoding::BitVector::fromBinary(bits);
}

std::string groupsJson(const std::vector<std::string>& groups) {
    std::string json = "[";
    for (std::size_t index = 0; index < groups.size(); ++index) {
        if (index != 0) {
            json += ",";
        }
        json += jsonString(groups[index]);
    }
    json += "]";
    return json;
}

std::string bitString(bool bit) {
    return bit ? "1" : "0";
}

template <typename Result>
void printSigned(
    const Result& result,
    bool decode,
    bool json
) {
    if (json) {
        std::cout
            << "{\"operation\":\""
            << (decode ? "signed-decode" : "signed-encode")
            << "\",\"value\":" << jsonString(result.value.str())
            << ",\"width\":" << result.width
            << ",\"encoding\":"
            << jsonString(encoding::signedEncodingName(result.encoding))
            << ",\"bits\":" << jsonString(result.bits.toBinary())
            << ",\"hex\":" << jsonString(result.bits.toHex())
            << ",\"representable\":true"
            << ",\"negativeZero\":"
            << (result.negativeZero ? "true" : "false")
            << ",\"minimum\":" << jsonString(result.range.minimum.str())
            << ",\"maximum\":" << jsonString(result.range.maximum.str())
            << "}\n";
        return;
    }

    std::cout
        << "value=" << result.value.str()
        << " bits=" << result.bits.toBinary()
        << " hex=" << result.bits.toHex()
        << " negativeZero=" << (result.negativeZero ? "true" : "false")
        << '\n';
}

void runSignedEncode(const EncodingOptions& options) {
    rejectUnknown(options, {"value", "width", "encoding"});

    const BigInt value = parseDecimal(
        required(options, "value"),
        "--value",
        true
    );
    const std::size_t width = parseWidth(options);
    const auto encodingMode = encoding::parseSignedEncoding(
        required(options, "encoding")
    );

    const auto result = encoding::encodeSigned(
        value,
        width,
        encodingMode,
        options.negativeZero
    );
    printSigned(result, false, options.json);
}

void runSignedDecode(const EncodingOptions& options) {
    rejectUnknown(options, {"bits", "encoding"});

    const auto bits = parseBitVector(required(options, "bits"), "--bits");
    const auto encodingMode = encoding::parseSignedEncoding(
        required(options, "encoding")
    );
    const auto result = encoding::decodeSigned(bits, encodingMode);
    printSigned(result, true, options.json);
}

void printBcd(
    const encoding::BcdResult& result,
    bool decode,
    bool json
) {
    if (json) {
        std::cout
            << "{\"operation\":\""
            << (decode ? "bcd-decode" : "bcd-encode")
            << "\",\"decimal\":" << jsonString(result.digits)
            << ",\"digits\":" << jsonString(result.digits)
            << ",\"bits\":" << jsonString(result.bits)
            << ",\"groups\":" << groupsJson(result.groups)
            << ",\"value\":" << jsonString(result.value.str())
            << ",\"binary\":" << jsonString(result.binary)
            << "}\n";
        return;
    }

    std::cout
        << "digits=" << result.digits
        << " bits=" << result.bits
        << " binary=" << result.binary
        << '\n';
}

void runBcdEncode(const EncodingOptions& options) {
    rejectUnknown(options, {"decimal"});

    const std::string& decimal = required(options, "decimal");
    if (decimal.size() > kMaxBcdDigits) {
        throw LimitError("--decimal exceeds the 1024-digit limit");
    }

    const auto result = encoding::encodeBcd(decimal);
    printBcd(result, false, options.json);
}

void runBcdDecode(const EncodingOptions& options) {
    rejectUnknown(options, {"bits"});

    const std::string& bitString = required(options, "bits");
    if (bitString.size() > kMaxBcdInputChars) {
        throw LimitError("--bits exceeds the BCD input limit");
    }

    std::size_t bitCount = 0;
    for (const char character : bitString) {
        if (character == ' ') {
            continue;
        }
        ++bitCount;
        if (bitCount > kMaxBcdBits) {
            throw LimitError(
                "--bits exceeds the 4096-bit BCD limit (1024 groups)"
            );
        }
    }

    const auto result = encoding::decodeBcd(bitString);
    printBcd(result, true, options.json);
}

void printGray(
    const encoding::GrayResult& result,
    bool decode,
    bool json
) {
    if (json) {
        std::cout
            << "{\"operation\":\""
            << (decode ? "gray-decode" : "gray-encode")
            << "\",\"value\":" << jsonString(result.value.str())
            << ",\"width\":" << result.width
            << ",\"binary\":" << jsonString(result.binary.toBinary())
            << ",\"gray\":" << jsonString(result.gray.toBinary())
            << ",\"binaryHex\":" << jsonString(result.binary.toHex())
            << ",\"grayHex\":" << jsonString(result.gray.toHex())
            << "}\n";
        return;
    }

    std::cout
        << "value=" << result.value.str()
        << " binary=" << result.binary.toBinary()
        << " gray=" << result.gray.toBinary()
        << '\n';
}

void runGrayEncode(const EncodingOptions& options) {
    rejectUnknown(options, {"value", "width"});

    const BigInt value = parseDecimal(
        required(options, "value"),
        "--value",
        true
    );
    const std::size_t width = parseWidth(options);
    const auto result = encoding::encodeGray(value, width);
    printGray(result, false, options.json);
}

void runGrayDecode(const EncodingOptions& options) {
    rejectUnknown(options, {"bits"});

    const auto gray = parseBitVector(required(options, "bits"), "--bits");
    const auto result = encoding::decodeGray(gray);
    printGray(result, true, options.json);
}

void runParity(const EncodingOptions& options) {
    rejectUnknown(options, {"bits", "parity", "flip-bit"});

    const auto data = parseBitVector(required(options, "bits"), "--bits");
    const auto mode = encoding::parseParityMode(
        required(options, "parity")
    );

    std::optional<std::size_t> flipIndex;
    const auto flip = options.values.find("flip-bit");
    if (flip != options.values.end()) {
        const int parsed = parseInteger<int>(flip->second, "--flip-bit");
        if (parsed < 0 || static_cast<std::size_t>(parsed) >= data.width()) {
            throw OptionError(
                "--flip-bit must be a bit index from 0 to " +
                std::to_string(data.width() - 1U)
            );
        }
        flipIndex = static_cast<std::size_t>(parsed);
    }

    const auto generated = encoding::generateParity(data, mode);
    const encoding::BitVector received = flipIndex.has_value()
        ? data.flipBit(*flipIndex)
        : data;
    const auto receivedParity = encoding::generateParity(received, mode);
    const bool valid = receivedParity.parityBit == generated.parityBit;

    if (options.json) {
        std::cout
            << "{\"operation\":\"parity\""
            << ",\"bits\":" << jsonString(received.toBinary())
            << ",\"originalBits\":" << jsonString(data.toBinary())
            << ",\"width\":" << data.width()
            << ",\"parity\":"
            << jsonString(encoding::parityModeName(mode))
            << ",\"onesCount\":" << generated.onesCount
            << ",\"receivedOnesCount\":" << receivedParity.onesCount
            << ",\"parityBit\":" << jsonString(bitString(generated.parityBit))
            << ",\"computedParityBit\":"
            << jsonString(bitString(receivedParity.parityBit))
            << ",\"dataWithParity\":"
            << jsonString(generated.dataWithParity)
            << ",\"receivedWithParity\":"
            << jsonString(received.toBinary() + bitString(generated.parityBit))
            << ",\"valid\":" << (valid ? "true" : "false")
            << ",\"flipped\":" << (flipIndex.has_value() ? "true" : "false")
            << ",\"flippedIndex\":";
        if (flipIndex.has_value()) {
            std::cout << *flipIndex;
        } else {
            std::cout << "null";
        }
        std::cout << "}\n";
        return;
    }

    std::cout
        << "valid=" << (valid ? "true" : "false")
        << " parityBit=" << bitString(generated.parityBit)
        << " receivedParityBit=" << bitString(receivedParity.parityBit)
        << " dataWithParity=" << generated.dataWithParity
        << '\n';
}

int dispatch(const EncodingOptions& options) {
    if (options.operation == "signed-encode") {
        runSignedEncode(options);
    } else if (options.operation == "signed-decode") {
        runSignedDecode(options);
    } else if (options.operation == "bcd-encode") {
        runBcdEncode(options);
    } else if (options.operation == "bcd-decode") {
        runBcdDecode(options);
    } else if (options.operation == "gray-encode") {
        runGrayEncode(options);
    } else if (options.operation == "gray-decode") {
        runGrayDecode(options);
    } else if (options.operation == "parity") {
        runParity(options);
    } else {
        throw OptionError(
            "Unknown operation: " + options.operation
        );
    }

    return 0;
}

} // namespace


void printEncodingHelp(std::ostream& stream) {
    stream
        << "Usage: radixlab-cli encoding --operation OP [options]\n"
        << "\n"
        << "Digital logic encodings. Add --json for structured output.\n"
        << "\n"
        << "Operations:\n"
        << "  signed-encode  --value INT --width N --encoding NAME\n"
        << "                 [--negative-zero]\n"
        << "  signed-decode  --bits BITS --encoding NAME\n"
        << "  bcd-encode     --decimal DIGITS\n"
        << "  bcd-decode     --bits BITS\n"
        << "  gray-encode    --value INT --width N\n"
        << "  gray-decode    --bits BITS\n"
        << "  parity         --bits BITS --parity even|odd [--flip-bit INDEX]\n"
        << "\n"
        << "Encoding names: SIGN_MAGNITUDE, ONES_COMPLEMENT, TWOS_COMPLEMENT\n"
        << "Widths: 1-64 bits. BCD accepts up to 1024 decimal digits.\n"
        << "\n"
        << "Examples:\n"
        << "  radixlab-cli encoding --operation signed-encode \\\n"
        << "    --value -13 --width 8 --encoding TWOS_COMPLEMENT --json\n"
        << "  radixlab-cli encoding --operation gray-encode \\\n"
        << "    --value 13 --width 8 --json\n"
        << "  radixlab-cli encoding --operation parity \\\n"
        << "    --bits 1011001 --parity EVEN --flip-bit 3 --json\n"
        << "\n"
        << "Exit codes:\n"
        << "  0  success\n"
        << "  1  internal error\n"
        << "  2  invalid arguments or input\n"
        << "  3  input resource limit exceeded\n"
        << "  4  value outside the representable range\n";
}


bool isEncodingCommand(std::string_view firstArgument) {
    return firstArgument == "encoding";
}


int runEncodingCommand(int argc, char* argv[]) {
    const bool jsonRequested = wantsJson(argc, argv);

    try {
        const EncodingOptions options = parseOptions(argc, argv);
        return dispatch(options);
    } catch (const LimitError& error) {
        emitError(jsonRequested, "input_too_large", error.what());
        return 3;
    } catch (const encoding::RangeError& error) {
        emitError(jsonRequested, "range_error", error.what());
        return 4;
    } catch (const OptionError& error) {
        emitError(jsonRequested, "invalid_request", error.what());
        return 2;
    } catch (const std::invalid_argument& error) {
        emitError(jsonRequested, "invalid_input", error.what());
        return 2;
    } catch (const std::out_of_range& error) {
        emitError(jsonRequested, "invalid_input", error.what());
        return 2;
    } catch (const std::exception& error) {
        emitError(jsonRequested, "internal_error", error.what());
        return 1;
    }
}

} // namespace cli
} // namespace radixlab
