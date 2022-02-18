#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

resolved_mechanism single_assign(const resolved_mechanism&);
r_expr single_assign(const r_expr&, std::unordered_set<std::string>& temps, std::unordered_map<std::string, r_expr>& rewrites);
r_expr single_assign(const r_expr&);

} // namespace resolved_ir
} // namespace al