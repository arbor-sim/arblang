#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

std::pair<resolved_mechanism, bool> cse(const resolved_mechanism&);
std::pair<r_expr, bool> cse(const r_expr&, std::unordered_map<resolved_expr, r_expr>& expr_map);
std::pair<r_expr, bool> cse(const r_expr&);

} // namespace resolved_ir
} // namespace al