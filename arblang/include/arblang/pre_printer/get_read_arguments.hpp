#pragma once

#include <string>
#include <unordered_map>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {
using state_field_map = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

// Append resolved_arguments that are being read to the provided vector
void read_arguments(const r_expr&, std::vector<std::string>&);

} // namespace resolved_ir
} // namespace al