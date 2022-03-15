#include <cassert>

#include <arblang/optimizer/optimizer.hpp>
#include <arblang/resolver/canonicalize.hpp>
#include <arblang/solver/solve.hpp>
#include <arblang/solver/solve_ode.hpp>
#include <arblang/solver/symbolic_diff.hpp>
#include <arblang/util/pretty_printer.hpp>

#include <fmt/core.h>

#include "../util/rexp_helpers.hpp"

namespace al {
namespace resolved_ir {

resolved_effect get_ig_pair(const resolved_effect& e, const std::string& v);

resolved_mechanism solve(const resolved_mechanism& e) {
    resolved_mechanism mech;
    if (!e.constants.empty()) {
        throw std::runtime_error("Internal compiler error, unexpected constant at this stage of the compiler");
    }
    if (!e.functions.empty()) {
        throw std::runtime_error("Internal compiler error, unexpected function at this stage of the compiler");
    }
    for (const auto& c: e.parameters) {
        mech.parameters.push_back(c);
    }
    for (const auto& c: e.states) {
        mech.states.push_back(c);
    }
    for (const auto& c: e.initializations) {
        mech.initializations.push_back(c);
    }
    for (const auto& c: e.evolutions) {
        // Solve the ODE of a resolved_evolve
        auto ev = std::get<resolved_evolve>(*c);
        auto s = solver(ev);
        mech.evolutions.push_back(make_rexpr<resolved_evolve>(s.solve()));
    }
    std::string v_sym = {};
    for (const auto& c: e.bindings) {
        mech.bindings.push_back(c);
        auto b = std::get<resolved_bind>(*c);
        if (b.bind == bindable::membrane_potential) {
            v_sym = b.name;
        }
    }
    // Add binding for dt
    mech.bindings.push_back(make_rexpr<resolved_bind>("dt", bindable::dt, std::optional<std::string>{},
                                                      make_rtype<resolved_quantity>(quantity::time, src_location{}),
                                                      src_location{}));
    for (const auto& c: e.effects) {
        // If the effect is a current or current_density affectable, rewrite to
        // current_density_pair or current_pair respectively containing the
        // {current_density, conductivity} and {current, conductance}
        // contributions respectively
        auto effect = std::get<resolved_effect>(*c);
        mech.effects.push_back(make_rexpr<resolved_effect>(get_ig_pair(effect, v_sym)));
    }
    for (const auto& c: e.exports) {
        mech.exports.push_back(c);
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return mech;
}

resolved_effect get_ig_pair(const resolved_effect& e, const std::string& v) {
    if (v.empty()) {
        return e;
    }
    if (e.effect != affectable::current_density && e.effect != affectable::current) {
        return e;
    }

    bool is_denisty = e.effect == affectable::current_density;

    // Extract the i value
    resolved_let concat_let;
    bool has_let = false;

    r_expr i = e.value;
    if (auto let_opt = get_let(e.value)) {
        auto let = let_opt.value();
        concat_let = let;
        i = get_innermost_body(&let);
        has_let = true;
    }

    auto g = sym_diff(i, v);
    // fix unit of g by dividing by a unit voltage

    auto voltage_type = make_rtype<resolved_quantity>(quantity::voltage, src_location{});
    auto unit_voltage = make_rexpr<resolved_int>(1, voltage_type, src_location{});
    g = make_rexpr<resolved_binary>(binary_op::div, g, unit_voltage, src_location{});

    // Combine i and g into a resolved_object
    std::string i_name = "i";
    std::string g_name = "g";
    if (e.ion) {
        i_name += ("_" + e.ion.value());
        g_name += ("_" + e.ion.value());
    }
    auto ig_type = make_rtype<resolved_record>(
            std::vector<std::pair<std::string, r_type>>{{i_name, type_of(i)}, {g_name, type_of(g)}}, src_location{});
    std::vector<r_expr> ig_fields = {
        make_rexpr<resolved_variable>(i_name, i, type_of(i), src_location{}),
        make_rexpr<resolved_variable>(g_name, g, type_of(g), src_location{})
    };
    auto ig_pair = make_rexpr<resolved_object>(ig_fields, ig_type, src_location{});

    r_expr solution;
    if (has_let) {
        set_innermost_body(&concat_let, ig_pair);
        solution = make_rexpr<resolved_let>(concat_let);
    } else {
        solution = ig_pair;
    }

    // Recanonicalize the derivatives with a new prefix to avoid name collisions.
    solution = canonicalize(solution, "i");

    // Reoptimize the obtained expressions.
    auto opt = optimizer(solution);
    solution = opt.optimize();

    return resolved_effect(is_denisty? affectable::current_density_pair: affectable::current_pair,
                           e.ion, solution, type_of(ig_pair), e.loc);
}

} // namespace resolved_ir
} // namespace al