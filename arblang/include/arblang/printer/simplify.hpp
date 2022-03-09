#pragma once

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

// Transform all resolved_quantity types and resolved_boolean types to real
// and all resolved_record field types to real
// replace resolved_field access with resolved_argument fields
printable_mechanism simplify(const resolved_mechanism&);

} // namespace resolved_ir
} // namespace al