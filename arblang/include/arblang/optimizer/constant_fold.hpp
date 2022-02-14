#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

resolved_mechanism constant_fold(const resolved_mechanism&);
r_expr constant_fold(const r_expr&, std::unordered_map<std::string, r_expr>& constants);
r_expr constant_fold(const r_expr&);

} // namespace resolved_ir
} // namespace al