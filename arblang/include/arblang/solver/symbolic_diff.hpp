#pragma once

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

struct diff_var {
    std::string sym;
    std::optional<std::string> sub_field;
    r_type type;
};
r_expr sym_diff(const r_expr&, const diff_var&);

} // namespace resolved_ir
} // namespace al