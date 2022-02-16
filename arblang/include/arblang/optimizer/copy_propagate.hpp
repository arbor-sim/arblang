#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

std::pair<resolved_mechanism, bool> copy_propagate(const resolved_mechanism&);
std::pair<r_expr, bool> copy_propagate(const r_expr&, std::unordered_map<std::string, r_expr>& copies);
std::pair<r_expr, bool> copy_propagate(const r_expr&);

} // namespace resolved_ir
} // namespace al