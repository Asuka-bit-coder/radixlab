#pragma once

#include <cstddef>

namespace radixlab::boolean {

// Parser-internal bound. Generated canonical expressions can be longer than a
// single user request, so the public CLI/API apply a stricter 1024-character
// input limit.
inline constexpr std::size_t kMaxExpressionLength = 4096;
inline constexpr std::size_t kMaxAstNodes = 4096;
inline constexpr std::size_t kMaxNestingDepth = 64;
inline constexpr std::size_t kMaxTruthTableVariables = 6;

} // namespace radixlab::boolean
