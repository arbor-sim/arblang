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

r_expr get_b(const resolved_evolve& e) {
    // Get the type and name of the state and create an equivalent zero-valued expression.
    // `state` is expected to be a resolved_argument.

    auto state_loc = location_of(e.identifier);
    auto state_type = type_of(e.identifier);
    std::string state_name;
    if (auto s = std::get_if<resolved_argument>(e.identifier.get())) {
        state_name = s->name;
    }
    else {
        throw std::runtime_error("Internal compiler error, expected a resolved_argument as the "
                                 "identifier of the resolved_evolve at " + to_string(state_loc));
    }

    r_expr zero_state;

    // if `state` is an object type, set each of the fields to 0.
    if (auto t = std::get_if<resolved_record>(state_type.get())) {
        std::vector<r_expr> fields;
        for (const auto& [f_id, f_type]: t->fields) {
            auto zero = make_rexpr<resolved_int>(0, f_type, state_loc);
            fields.push_back(make_rexpr<resolved_variable>(f_id, zero, f_type, state_loc));
        }
        zero_state = make_rexpr<resolved_object>(fields, state_type, state_loc);
    }
    else {
        zero_state = make_rexpr<resolved_int>(0, state_type, state_loc);
    }

    // Use the copy_propagate function to propagate the zero_state.
    std::unordered_map<std::string, r_expr> copy_map = {{state_name, zero_state}};
    std::unordered_map<std::string, r_expr> rewrite_map = {};

    // Rerun optimizations to propagate the zero constant and simplify the expressions.
    auto e_copy = copy_propagate(e.value, copy_map, rewrite_map);

    auto opt = optimizer(e_copy.first);
    return opt.optimize();
}

r_expr get_a(const resolved_evolve& e) {
    // Differentiate the evolve statement value w.r.t the state

    auto state_loc = location_of(e.identifier);
    auto state_type = type_of(e.identifier);
    std::string state_name;
    if (auto s = std::get_if<resolved_argument>(e.identifier.get())) {
        state_name = s->name;
    }
    else {
        throw std::runtime_error("Internal compiler error, expected a resolved_argument as the "
                                 "identifier of the resolved_evolve at " + to_string(state_loc));
    }

    // We only need the innermost result (body of the deepest let statement).
    // We can differentiate only the values referred to by the used resolved_variables.
    r_expr evolve_result = e.value;
    if (auto let = std::get_if<resolved_let>(e.value.get())) {
        evolve_result = get_innermost_body(let);
    }

    r_expr e_deriv;
    // The state is expected to be either a resolved_object or a resolved_variable
    if (std::get_if<resolved_record>(state_type.get())) {
        auto obj = std::get_if<resolved_object>(evolve_result.get());
        if (!obj) {
            throw std::runtime_error("Internal compiler error, expected a resolved_object as the "
                                     "result of the resolved_evolve at " + to_string(state_loc));
        }

        std::vector<r_expr> field_deriv;
        for (const auto& field: obj->record_fields) {
            auto fld = std::get_if<resolved_variable>(field.get());
            if (!fld) {
                throw std::runtime_error("Internal compiler error, expected a resolved_varialbe as the "
                                         "field of the resolved_object at " + to_string(obj->loc));
            }

            // users could define their own records for this, but then associating
            // the fields to one another becomes ambiguous.
            auto f_name = fld->name;
            if (f_name.back() != '\'') {
                throw std::runtime_error("Internal compiler error, expected a \' as the identifier of the state_field at " + to_string(obj->loc));
            }
            f_name.pop_back();

            // For each field, differentiate w.r.t the state name and the field name
            field_deriv.push_back(make_rexpr<resolved_variable>(fld->name, sym_diff(fld->value, state_name, f_name), fld->type, fld->loc));
        }
        e_deriv = make_rexpr<resolved_object>(field_deriv, obj->type, obj->loc);
    }
    else {
        e_deriv = sym_diff(evolve_result, state_name);
    }

    // Recanonicalize the derivatives with a new prefix to avoid name collisions.
    e_deriv = canonicalize(e_deriv, "d");

    // Reoptimize the obtained expressions.
    auto opt_a = optimizer(e_deriv);
    return opt_a.optimize();
}

r_expr generate_solution(const r_expr& a, const r_expr& b, const r_expr& x) {
    // Solving x' = x*a + b

    // TODO, check monolinearity!
    auto a_opt = as_number(a);
    auto b_opt = as_number(b);

    // TODO doesn't have real type.
    auto empty_loc = src_location{};
    auto real_type = make_rtype<resolved_quantity>(normalized_type(quantity::real), empty_loc);
    auto dt = make_rexpr<resolved_argument>("dt", real_type, empty_loc);

    if (a_opt && (a_opt.value() == 0)) {
        // x' = b becomes x = x + b*dt;
        // TODO doesn't have to have real type.
        auto b_dt = make_rexpr<resolved_binary>(binary_op::mul, b, dt, real_type, empty_loc);
        return make_rexpr<resolved_binary>(binary_op::add, x, b_dt, real_type, empty_loc);
    }
    if (b_opt && (b_opt.value() == 0)) {
        // x' = a*x becomes x = x*exp(a*dt);
        // TODO doesn't have to have real type.
        auto a_dt = make_rexpr<resolved_binary>(binary_op::mul, a, dt, real_type, empty_loc);
        auto exp_a_dt = make_rexpr<resolved_unary>(unary_op::exp, a_dt, real_type, empty_loc);
        return make_rexpr<resolved_binary>(binary_op::add, x, exp_a_dt, real_type, empty_loc);
    }
    // x' = a*x + b becomes x = -b/a + (x+b/a)*exp(a*dt);

    // TODO doesn't have to have real type.
    auto b_div_a  = make_rexpr<resolved_binary>(binary_op::div, b, a, real_type, empty_loc);
    auto a_mul_dt = make_rexpr<resolved_binary>(binary_op::mul, a, dt, real_type, empty_loc);
    auto exp_term = make_rexpr<resolved_unary>(unary_op::exp, a_mul_dt, real_type, empty_loc);
    auto add_term = make_rexpr<resolved_binary>(binary_op::add, x, b_div_a, real_type, empty_loc);
    auto mul_term = make_rexpr<resolved_binary>(binary_op::mul, add_term, exp_term, real_type, empty_loc);
    auto neg_term = make_rexpr<resolved_unary>(unary_op::neg, b_div_a, real_type, empty_loc);
    return make_rexpr<resolved_binary>(binary_op::add, neg_term, mul_term, real_type, empty_loc);

    // TODO  Special case when x' = (b-x)/a; rather
    // than implement more general algebraic simplification, jump
    // straight to simplified update: x = b + (x-b)*exp(-dt/a).
}

resolved_evolve solve_ode(const resolved_evolve& e) {
    // resolved_evolve statement is expected to be and ODE of the form x' = x*a + b

    auto b_expr = get_b(e);
    std::cout << "b_expr:\n" << pretty_print(b_expr) << std::endl;

    auto a_expr = get_a(e);
    std::cout << "a_expr:\n" << pretty_print(a_expr) << std::endl;

    // a and b ae expected to be either a single expression or a list of nested let statements.
    // We can extract the innermost results representing a and b, and concatenate the surrounding lets.

    resolved_let concat_let;
    bool has_let = false;

    r_expr b_inner = b_expr;
    r_expr a_inner = a_expr;
    if (auto let_opt = get_let(b_expr)) {
        auto let = let_opt.value();
        concat_let = let;
        b_inner = get_innermost_body(&let);
        has_let = true;
    }
    if (auto let_opt = get_let(a_expr)) {
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
    auto a_obj = std::get_if<resolved_object>(a_inner.get());
    auto b_obj = std::get_if<resolved_object>(b_inner.get());
    auto b_var = std::get_if<resolved_variable>(b_inner.get());
    auto a_var = std::get_if<resolved_variable>(a_inner.get());

    if (a_obj && b_obj) {
        assert(a_obj->record_fields.size() == b_obj->record_fields.size());

        std::vector<r_expr> fields;
        for (unsigned i = 0; i < a_obj->record_fields.size(); ++i) {
            auto a_field = std::get_if<resolved_variable>(a_obj->record_fields[i].get());
            auto b_field = std::get_if<resolved_variable>(b_obj->record_fields[i].get());

            if (!a_field || !b_field) {
                throw std::runtime_error("Internal compiler error, expected a resolved_variable as the "
                                         "identifiers of the state in the ODE solver");
            }

            auto a_val  = a_field->value;
            auto a_name = a_field->name;

            auto b_val  = b_field->value;
            auto b_name = b_field->name;

            // Solve each field separately
            if ((a_name != b_name) || (a_name.back() != '\'')) {
                throw std::runtime_error("Internal compiler error, expected a \' as the identifier of the "
                                         "state_field at " + to_string(a_field->loc));
            }
            a_name.pop_back();

            bool found = false;
            auto state_type = std::get<resolved_record>(*type_of(e.identifier));
            for (const auto& [f_id, f_type]: state_type.fields) {
                if (f_id == a_name) {
                    auto s_val = make_rexpr<resolved_field_access>(e.identifier, a_name, f_type, e.loc);
                    fields.push_back(make_rexpr<resolved_variable>(a_name, generate_solution(a_val, b_val, s_val), f_type, a_field->loc));
                    found = true;
                    break;
                }
            }
            if (!found) {
                throw std::runtime_error("Internal compiler error, something went wrong when solving the ODE at " + to_string(e.loc));
            }
        }
        solution = make_rexpr<resolved_object>(fields, type_of(e.identifier), location_of(e.identifier));
    }
    else if (a_var && b_var) {
        auto a_val  = a_var->value;
        auto b_val  = b_var->value;
        solution = generate_solution(a_val, b_val, e.identifier);
    }

    // Recanonicalize the derivatives with a new prefix to avoid name collisions.
    solution = canonicalize(solution, "s");

    // Reoptimize the obtained expressions.
    auto opt = optimizer(solution);
    solution = opt.optimize();

    if (!has_let) return resolved_evolve(e.identifier, solution, type_of(e.identifier), e.loc);

    set_innermost_body(&concat_let, solution);
    return resolved_evolve(e.identifier, make_rexpr<resolved_let>(concat_let), type_of(e.identifier), e.loc);
}

} // namespace resolved_ir
} // namespace al