#pragma once

#include "arblang/resolver/resolved_expressions.hpp"
#include "arblang/resolver/resolved_types.hpp"

namespace al {
using namespace resolved_type_ir;
using namespace resolved_ir;

std::string pretty_print(const resolved_mechanism&);
std::string pretty_print(const r_expr&);
std::string pretty_print(const r_type&);

std::string expand(const r_expr&, int indent = 0);
}