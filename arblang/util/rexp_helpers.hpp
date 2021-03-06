#pragma once

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {
r_expr get_innermost_body(resolved_let* let);
void set_innermost_body(resolved_let* let, const r_expr& body);

std::optional<double> is_number(const r_expr& e);
bool is_trivial(const r_expr& e);

} // namespace resolved_ir
} // namespace al
