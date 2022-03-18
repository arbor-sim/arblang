#pragma once

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

resolved_mechanism solve(const resolved_mechanism& e, const std::string& i_name, const std::string& g_name);

} // namespace resolved_ir
} // namespace al