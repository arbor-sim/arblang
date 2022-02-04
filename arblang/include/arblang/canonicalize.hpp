#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

resolved_mechanism canonicalize(const resolved_mechanism&);
r_expr canonicalize(const r_expr&, std::unordered_set<std::string>& temps);
r_expr canonicalize(const r_expr&);

} // namespace resolved_ir
} // namespace al