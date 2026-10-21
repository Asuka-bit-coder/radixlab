#pragma once

#include <iosfwd>
#include <string_view>

namespace radixlab {
namespace cli {

void printBooleanHelp(std::ostream& stream);

[[nodiscard]]
bool isBooleanCommand(std::string_view firstArgument);

int runBooleanCommand(int argc, char* argv[]);

} // namespace cli
} // namespace radixlab
