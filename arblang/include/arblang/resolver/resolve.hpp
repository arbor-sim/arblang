#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

struct in_scope_map {
    std::unordered_map<std::string, r_expr> param_map;
    std::unordered_map<std::string, r_expr> const_map;
    std::unordered_map<std::string, r_expr> state_map;
    std::unordered_map<std::string, r_expr> bind_map;
    std::unordered_map<std::string, r_expr> local_map;
    std::unordered_map<std::string, r_expr> func_map;
    std::unordered_map<std::string, r_type> type_map;
};

resolved_mechanism resolve(const parsed_ir::parsed_mechanism&);
r_expr resolve(const parsed_ir::p_expr &, const in_scope_map&);

} // namespace resolved_ir
} // namespace al