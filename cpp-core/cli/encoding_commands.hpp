#pragma once

#include <iosfwd>
#include <string_view>

namespace radixlab {
namespace cli {

void printEncodingHelp(std::ostream& stream);

[[nodiscard]]
bool isEncodingCommand(std::string_view firstArgument);

int runEncodingCommand(int argc, char* argv[]);

} // namespace cli
} // namespace radixlab
