#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

resolved_evolve solve_ode(const resolved_evolve&);

} // namespace resolved_ir
} // namespace al