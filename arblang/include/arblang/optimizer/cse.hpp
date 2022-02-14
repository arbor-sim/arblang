#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

resolved_mechanism cse(const resolved_mechanism&);
r_expr cse(const r_expr&, std::unordered_map<resolved_expr, r_expr>& expr_map);
r_expr cse(const r_expr&);

} // namespace resolved_ir
} // namespace al