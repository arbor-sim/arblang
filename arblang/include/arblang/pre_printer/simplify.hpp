#pragma once

#include <string>
#include <unordered_map>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {
using state_field_map = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

// resolved_quantity and resolved_boolean are simplified to real resolved_quantity
// resolved_record is simplified to resolved_record with real fields
// resolved_field_access is simplified to resolved_argument from the state_field_map.
r_expr simplify(const r_expr&, const state_field_map&);
r_type simplify(const r_type&);

} // namespace resolved_ir
} // namespace al