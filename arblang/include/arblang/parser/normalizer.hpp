#pragma once

#include <optional>
#include <string>
#include <vector>

#include <arblang/parser/parsed_expressions.hpp>

namespace al {
namespace parsed_ir {

parsed_mechanism normalize(const parsed_mechanism& e);
p_expr normalize(const p_expr& e);

} // namespace parsed_ir
} // namespace al