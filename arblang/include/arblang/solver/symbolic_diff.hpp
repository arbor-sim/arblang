#pragma once

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

// TODO types are a big Question Mark in the implementation.
r_expr sym_diff(const r_expr&, const std::string& sym, const std::optional<std::string>& field = {});

} // namespace resolved_ir
} // namespace al