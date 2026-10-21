#include "radixlab/encoding/signed_encoding.hpp"

#include "radixlab/encoding/encoding_error.hpp"

#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>

namespace radixlab::encoding {

namespace {

void validateWidth(std::size_t width) {
    if (width < 1 || width > BitVector::kMaxWidth) {
        throw std::invalid_argument("Signed encoding width must be 1-64 bits");
    }
}


BigInt modulusFor(std::size_t width) {
    return BigInt{1} << static_cast<unsigned int>(width);
}


BigInt magnitudeMaskFor(std::size_t width) {
    return (BigInt{1} << static_cast<unsigned int>(width - 1U)) - 1;
}


BigInt allOnesFor(std::size_t width) {
    return modulusFor(width) - 1;
}


std::string normalizedName(std::string_view name) {
    std::string normalized;
    normalized.reserve(name.size());

    for (const char character : name) {
        const unsigned char byte = static_cast<unsigned char>(character);
        if (character == '-' || character == '_' || character == ' ') {
            continue;
        }
        normalized.push_back(
            static_cast<char>(std::tolower(byte))
        );
    }

    return normalized;
}

} // namespace


SignedEncoding parseSignedEncoding(std::string_view name) {
    const std::string normalized = normalizedName(name);

    if (normalized == "signmagnitude" || normalized == "sm") {
        return SignedEncoding::SignMagnitude;
    }

    if (normalized == "onescomplement" || normalized == "ones") {
        return SignedEncoding::OnesComplement;
    }

    if (normalized == "twoscomplement" || normalized == "twos") {
        return SignedEncoding::TwosComplement;
    }

    throw std::invalid_argument(
        "Unknown signed encoding; use SIGN_MAGNITUDE, "
        "ONES_COMPLEMENT, or TWOS_COMPLEMENT"
    );
}


std::string_view signedEncodingName(SignedEncoding encoding) noexcept {
    switch (encoding) {
        case SignedEncoding::SignMagnitude:
            return "SIGN_MAGNITUDE";
        case SignedEncoding::OnesComplement:
            return "ONES_COMPLEMENT";
        case SignedEncoding::TwosComplement:
            return "TWOS_COMPLEMENT";
    }

    return "UNKNOWN";
}


SignedRange signedRange(std::size_t width, SignedEncoding encoding) {
    validateWidth(width);

    const BigInt magnitudeMaximum = magnitudeMaskFor(width);

    if (encoding == SignedEncoding::TwosComplement) {
        return SignedRange{
            .minimum = -magnitudeMaximum - 1,
            .maximum = magnitudeMaximum
        };
    }

    return SignedRange{
        .minimum = -magnitudeMaximum,
        .maximum = magnitudeMaximum
    };
}


SignedEncodeResult encodeSigned(
    BigInt value,
    std::size_t width,
    SignedEncoding encoding,
    bool negativeZero
) {
    const SignedRange range = signedRange(width, encoding);

    if (negativeZero) {
        if (encoding == SignedEncoding::TwosComplement) {
            throw std::invalid_argument(
                "Two's complement has a unique zero; negative zero is unavailable"
            );
        }

        if (value != 0) {
            throw std::invalid_argument(
                "negativeZero may only be requested when value is zero"
            );
        }
    }

    if (value < range.minimum || value > range.maximum) {
        throw RangeError(
            "Value " + value.str() + " is outside the representable range [" +
            range.minimum.str() + ", " + range.maximum.str() + "]"
        );
    }

    const BigInt modulus = modulusFor(width);
    const BigInt signBit =
        BigInt{1} << static_cast<unsigned int>(width - 1U);
    const BigInt mask = modulus - 1;

    BigInt pattern = 0;

    switch (encoding) {
        case SignedEncoding::SignMagnitude:
            if (negativeZero) {
                pattern = signBit;
            } else if (value < 0) {
                pattern = signBit + (-value);
            } else {
                pattern = value;
            }
            break;

        case SignedEncoding::OnesComplement:
            if (negativeZero) {
                pattern = mask;
            } else if (value < 0) {
                pattern = mask ^ (-value);
            } else {
                pattern = value;
            }
            break;

        case SignedEncoding::TwosComplement:
            if (value < 0) {
                pattern = modulus + value;
            } else {
                pattern = value;
            }
            break;
    }

    return SignedEncodeResult{
        .value = value,
        .width = width,
        .encoding = encoding,
        .bits = BitVector{std::move(pattern), width},
        .negativeZero = negativeZero,
        .range = range
    };
}


SignedDecodeResult decodeSigned(
    const BitVector& bits,
    SignedEncoding encoding
) {
    const std::size_t width = bits.width();
    const SignedRange range = signedRange(width, encoding);

    const BigInt pattern = bits.pattern();
    const BigInt signBit =
        BigInt{1} << static_cast<unsigned int>(width - 1U);
    const BigInt magnitudeMask = signBit - 1;
    const bool negative = bits.signBit();

    BigInt value = 0;
    bool negativeZero = false;

    switch (encoding) {
        case SignedEncoding::SignMagnitude: {
            const BigInt magnitude = pattern & magnitudeMask;
            negativeZero = negative && magnitude == 0;
            value = negative ? -magnitude : magnitude;
            break;
        }

        case SignedEncoding::OnesComplement: {
            if (negative) {
                const BigInt magnitude = allOnesFor(width) ^ pattern;
                negativeZero = magnitude == 0;
                value = -magnitude;
            } else {
                value = pattern;
            }
            break;
        }

        case SignedEncoding::TwosComplement:
            value = negative ? pattern - modulusFor(width) : pattern;
            break;
    }

    return SignedDecodeResult{
        .value = std::move(value),
        .width = width,
        .encoding = encoding,
        .bits = bits,
        .negativeZero = negativeZero,
        .range = range
    };
}

} // namespace radixlab::encoding
