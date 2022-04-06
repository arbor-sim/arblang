#include <cassert>
#include <string>
#include <unordered_map>

#include <arblang/resolver/canonicalize.hpp>
#include <arblang/optimizer/optimizer.hpp>
#include <arblang/solver/solve_ode.hpp>
#include <arblang/solver/symbolic_diff.hpp>
#include <arblang/util/pretty_printer.hpp>

#include <fmt/core.h>

#include "../util/rexp_helpers.hpp"

namespace al {
namespace resolved_ir {

solver::solver(const resolved_evolve& e):
    evolve(e),
    state_id(e.identifier),
    state_type(type_of(state_id)),
    state_loc(location_of(state_id))
{
    // The identifier of the evolve_expression is expected to be a
    // resolved_argument referring to a state variable.
    auto arg = is_resolved_argument(state_id);
    if (!arg) {
        throw std::runtime_error("Internal compiler error, expected a resolved_argument as the "
                                 "identifier of the resolved_evolve at " + to_string(state_loc));
    }
    state_name = arg->name;

    // The rhs of the ODE
    state_deriv = e.value;
    state_deriv_body = state_deriv;
    if (auto let = is_resolved_let(state_deriv)) {
        state_deriv_body = get_innermost_body(&let.value());
    }
}

r_expr solver::make_zero_state() {
    // if the state variable is an object type, set each of the fields to 0.
    if (is_resolved_record_type(state_type)) {
        auto rec = is_resolved_record_type(state_type).value();

        std::vector<r_expr> fields;
        for (const auto& [f_id, f_type]: rec.fields) {
            auto zero = make_rexpr<resolved_int>(0, f_type, state_loc);
            fields.push_back(make_rexpr<resolved_variable>(f_id, zero, f_type, state_loc));
        }
        return make_rexpr<resolved_object>(fields, state_type, state_loc);
    }
    else {
        return make_rexpr<resolved_int>(0, state_type, state_loc);
    }
}

r_expr solver::get_b() {
    r_expr zero_state = make_zero_state();

    // Use the copy_propagate function to propagate the zero_state.
    std::unordered_map<std::string, r_expr> copy_map = {{state_name, zero_state}};
    auto e_copy = copy_propagate(state_deriv, copy_map);

    return e_copy.first;
}

r_expr solver::get_a() {
    // Differentiate the evolve statement value w.r.t the state

    // We only need the innermost result (body of the deepest let statement)
    // because we can reconstruct the entire expression from that innermost
    // result.

    r_expr e_symdiff;
    // The state is expected to be either a resolved_object or a resolved_variable
    auto record_state = is_resolved_record_type(state_type);
    if (record_state) {
        auto obj = is_resolved_object(state_deriv_body);
        if (!obj) {
            throw std::runtime_error("Internal compiler error, expected a resolved_object as the "
                                     "result of the resolved_evolve at " + to_string(state_loc));
        }

        // If that state is a resolved object, we need to differentiate
        // each of the fields w.r.t the corresponding state field.
        std::vector<r_expr> field_symdiff;
        for (const auto& field: obj->record_fields) {
            auto fld = is_resolved_variable(field);
            if (!fld) {
                throw std::runtime_error("Internal compiler error, expected a resolved_varialbe as the "
                                         "field of the resolved_object at " + to_string(obj->loc));
            }

            // users could define their own records for this, but then associating
            // the object fields to one the state fields becomes ambiguous.
            // TODO look more into this.
            auto f_name = fld->name;
            if (f_name.back() != '\'') {
                throw std::runtime_error("Internal compiler error, expected a \' as the identifier of the "
                                         "state_field at " + to_string(obj->loc));
            }
            f_name.pop_back();

            r_type f_type;
            for (const auto& [name, type]: record_state->fields) {
                if (f_name == name) {
                    f_type = type;
                    break;
                }
            }
            if (!f_type) {
                throw std::runtime_error(fmt::format("Internal compiler error, expected to find field {} in type {}",
                                                     f_name, to_string(obj->loc)));
            }

            // For each field, differentiate w.r.t the state name and the field name
            auto f_symdiff = sym_diff(fld->value, {state_name, f_name, f_type});
            field_symdiff.push_back(make_rexpr<resolved_variable>(fld->name, f_symdiff, fld->type, fld->loc));
        }
        e_symdiff = make_rexpr<resolved_object>(field_symdiff, obj->type, obj->loc);
    }
    else {
        e_symdiff = sym_diff(state_deriv_body, {state_name, {}, state_type});
    }

    // Recanonicalize the derivatives with a new prefix to avoid name collisions.
    e_symdiff = canonicalize(e_symdiff, "d");

    // Reoptimize the obtained expressions.
    auto opt_a = optimizer(e_symdiff);
    return opt_a.optimize();
}

r_expr solver::generate_solution(const r_expr& a, const r_expr& b, const r_expr& x) {
    // Solving x' = x*a + b

    // TODO: check monolinearity! This is needed to make sure we don't attempt to
    //       solve systems of ODEs that are not diagonal linear

    auto a_opt = is_number(optimizer(a).optimize());
    auto b_opt = is_number(optimizer(b).optimize());

    auto empty_loc = src_location{};
    auto time_type = make_rtype<resolved_quantity>(normalized_type(quantity::time), empty_loc);
    auto dt = make_rexpr<resolved_argument>("dt", time_type, empty_loc);

    if (a_opt && (a_opt.value() == 0)) {
        // x' = b becomes x = x + b*dt;
        auto b_dt = make_rexpr<resolved_binary>(binary_op::mul, b, dt, empty_loc);
        return make_rexpr<resolved_binary>(binary_op::add, x, b_dt, empty_loc);
    }
    if (b_opt && (b_opt.value() == 0)) {
        // x' = a*x becomes x = x*exp(a*dt);
        auto a_dt = make_rexpr<resolved_binary>(binary_op::mul, a, dt, empty_loc);
        auto exp_a_dt = make_rexpr<resolved_unary>(unary_op::exp, a_dt, empty_loc);
        return make_rexpr<resolved_binary>(binary_op::add, x, exp_a_dt, empty_loc);
    }
    // x' = a*x + b becomes x = -b/a + (x+b/a)*exp(a*dt);

    auto b_div_a  = make_rexpr<resolved_binary>(binary_op::div, b, a, empty_loc);
    auto a_mul_dt = make_rexpr<resolved_binary>(binary_op::mul, a, dt, empty_loc);

    // instead of exp use the pade approximation: exp(t) = (1+0.5*t)/(1-0.5*t)
    auto half           = make_rexpr<resolved_float>(0.5, make_rtype<resolved_quantity>(quantity::real, empty_loc), empty_loc);
    auto half_a_mul_dt  = make_rexpr<resolved_binary>(binary_op::mul, half, a_mul_dt, empty_loc);
    auto one            = make_rexpr<resolved_float>(1., type_of(half_a_mul_dt), empty_loc);
    auto one_minus_term = make_rexpr<resolved_binary>(binary_op::sub, one, half_a_mul_dt, empty_loc);
    auto one_plus_term  = make_rexpr<resolved_binary>(binary_op::add, one, half_a_mul_dt, empty_loc);
    auto exp_term = make_rexpr<resolved_binary>(binary_op::div, one_plus_term, one_minus_term, empty_loc);

    auto add_term = make_rexpr<resolved_binary>(binary_op::add, x, b_div_a, empty_loc);
    auto mul_term = make_rexpr<resolved_binary>(binary_op::mul, add_term, exp_term, empty_loc);
    auto neg_term = make_rexpr<resolved_unary>(unary_op::neg, b_div_a, empty_loc);
    return make_rexpr<resolved_binary>(binary_op::add, neg_term, mul_term, empty_loc);

    // TODO: Special case when x' = (b-x)/a; rather than
    //       implement more general algebraic simplification, jump
    //       straight to simplified update: x = b + (x-b)*exp(-dt/a).
}

resolved_evolve solver::solve() {
    // resolved_evolve statement is expected to be and ODE of the form x' = x*a + b

    auto b_expr = get_b();
    auto a_expr = get_a();

    // a and b are expected to be either a single expression or a list of nested let statements.
    // We can extract the innermost results representing a and b, and concatenate the surrounding lets.

    resolved_let concat_let;
    bool has_let = false;

    r_expr b_inner = b_expr;
    r_expr a_inner = a_expr;
    if (auto let_opt = is_resolved_let(b_expr)) {
        auto let = let_opt.value();
        concat_let = let;
        b_inner = get_innermost_body(&let);
        has_let = true;
    }
    if (auto let_opt = is_resolved_let(a_expr)) {
        auto let = let_opt.value();
        if (!has_let) {
            concat_let = let;
        } else {
            set_innermost_body(&concat_let, a_expr);
        }
        a_inner = get_innermost_body(&let);
        has_let = true;
    }

    // Form solution
    r_expr solution;
    // if the state variable is a record type, a and b will be record types.
    auto a_obj = is_resolved_object(a_inner);
    auto b_obj = is_resolved_object(b_inner);
    auto b_var = is_resolved_variable(b_inner);
    auto a_var = is_resolved_variable(a_inner);

    if (a_obj && b_obj) {
        assert(a_obj->record_fields.size() == b_obj->record_fields.size());

        std::vector<r_expr> fields;
        for (unsigned i = 0; i < a_obj->record_fields.size(); ++i) {
            auto a_field = is_resolved_variable(a_obj->record_fields[i]);
            auto b_field = is_resolved_variable(b_obj->record_fields[i]);

            if (!a_field || !b_field) {
                throw std::runtime_error("Internal compiler error, expected a resolved_variable as the "
                                         "identifier of the state in the ODE solver");
            }

            auto a_val  = a_field->value;
            auto a_name = a_field->name;

            auto b_val  = b_field->value;
            auto b_name = b_field->name;

            // Solve each field separately
            if ((a_name != b_name) || (a_name.back() != '\'')) {
                throw std::runtime_error(fmt::format("Internal compiler error, expected a \' at the end of the name of the "
                                         "state_field at {}", to_string(a_field->loc)));
            }
            a_name.pop_back();

            bool found = false;
            auto stype = is_resolved_record_type(state_type).value();
            for (const auto& [f_id, f_type]: stype.fields) {
                if (f_id == a_name) {
                    auto s_val = make_rexpr<resolved_field_access>(state_id, a_name, f_type, state_loc);
                    fields.push_back(make_rexpr<resolved_variable>(a_name, generate_solution(a_val, b_val, s_val), f_type, a_field->loc));
                    found = true;
                    break;
                }
            }
            if (!found) {
                throw std::runtime_error(fmt::format("Internal compiler error, something went wrong when solving "
                                                     "the ODE at {}", to_string(evolve.loc)));
            }
        }
        solution = make_rexpr<resolved_object>(fields, state_type, state_loc);
    }
    else if (a_var && b_var) {
        auto a_val  = a_var->value;
        auto b_val  = b_var->value;
        solution = generate_solution(a_val, b_val, state_id);
    }

    // Recanonicalize the derivatives with a new prefix to avoid name collisions.
    solution = canonicalize(solution, "s");

    // Optimize the obtained expressions before returning.
    if (!has_let) return resolved_evolve(state_id, optimizer(solution).optimize(), state_type, evolve.loc);

    set_innermost_body(&concat_let, solution);
    auto opt  = optimizer(make_rexpr<resolved_let>(concat_let));
    return resolved_evolve(state_id, opt.optimize(), state_type, evolve.loc);
}

} // namespace resolved_ir
} // namespace al