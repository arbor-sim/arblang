#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

std::pair<resolved_mechanism, bool> constant_fold(const resolved_mechanism&);
std::pair<r_expr, bool> constant_fold(const r_expr&, std::unordered_map<std::string, r_expr>& constants);
std::pair<r_expr, bool> constant_fold(const r_expr&);

} // namespace resolved_ir
} // namespace al