#pragma once

#include <stdexcept>

namespace radixlab::encoding {

/**
 * Raised when a numeric value cannot be represented in the requested
 * encoding and width. This is distinct from malformed input.
 */
class RangeError : public std::out_of_range {
public:
    using std::out_of_range::out_of_range;
};

} // namespace radixlab::encoding
