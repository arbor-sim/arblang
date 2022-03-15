#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

resolved_mechanism inline_func(const resolved_mechanism&);
r_expr inline_func(const r_expr&,
                   std::unordered_set<std::string>& temps,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_func,
                   const std::string& pref);
r_expr inline_func(const r_expr&, std::unordered_map<std::string, r_expr>&, const std::string& pref);

} // namespace resolved_ir
} // namespace al