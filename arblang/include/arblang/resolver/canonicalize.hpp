#pragma once

#include <string>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

resolved_mechanism canonicalize(const resolved_mechanism&);
r_expr canonicalize(const r_expr&,
                    std::unordered_set<std::string>& temps,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref);
r_expr canonicalize(const r_expr&,
                    std::unordered_set<std::string>& temps,
                    const std::string& pref);
r_expr canonicalize(const r_expr&, const std::string& pref);

} // namespace resolved_ir
} // namespace al