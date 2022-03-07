#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

class solver {
private:
    resolved_evolve evolve; // modified resolved_evolve

    r_expr state_id;         // state expression
    r_type state_type;       // state type
    std::string state_name;  // state name
    src_location state_loc;  // state location

    r_expr state_deriv;      // state derivative
    r_expr state_deriv_body; // innermost body of state_derive

    bool record_state;       // whether or not the state has a record type

    r_expr make_zero_state();
public:
    solver(const resolved_evolve& e);
    r_expr get_b();
    r_expr get_a();
    r_expr generate_solution(const r_expr& a, const r_expr& b, const r_expr&);
    resolved_evolve solve();
};

} // namespace resolved_ir
} // namespace al