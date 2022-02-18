#pragma once

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {
r_expr get_innermost_body(resolved_let* let);
void set_innermost_body(resolved_let* let, const r_expr& body);
} // namespace resolved_ir
} // namespace al
