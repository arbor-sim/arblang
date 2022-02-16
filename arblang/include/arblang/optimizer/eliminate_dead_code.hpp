#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

std::pair<resolved_mechanism, bool> eliminate_dead_code(const resolved_mechanism&);
std::pair<r_expr, bool> eliminate_dead_code(const r_expr&);

void find_dead_code(const r_expr&, std::unordered_set<std::string>&);
r_expr remove_dead_code(const r_expr&, const std::unordered_set<std::string>&);

} // namespace resolved_ir
} // namespace al