#include "cli_io.hpp"

#include <iostream>

namespace radixlab {
namespace cli {

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


std::string jsonString(std::string_view text) {
    return "\"" + jsonEscape(text) + "\"";
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


bool wantsJson(int argc, char* argv[]) {
    for (int index = 1; index < argc; ++index) {
        if (std::string_view(argv[index]) == "--json") {
            return true;
        }
    }
    return false;
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

} // namespace cli
} // namespace radixlab
