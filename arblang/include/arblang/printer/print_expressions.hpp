#pragma once

#include <string>
#include <sstream>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

void print_expression(const r_expr&, std::stringstream&, const std::string& indent="");

} // namespace resolved_ir
} // namespace al