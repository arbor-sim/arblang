#include <arblang/optimizer/constant_fold.hpp>
#include <arblang/resolver/resolved_expressions.hpp>
#include <arblang/solver/symbolic_diff.hpp>

#include <../util/rexp_helpers.hpp>

#include <fmt/core.h>

namespace al {
namespace resolved_ir {

r_expr sym_diff(const resolved_record_alias& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation (after resolution).");
}

r_expr sym_diff(const resolved_function& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_function at "
                             "this stage in the compilation (after inlining).");
}

r_expr sym_diff(const resolved_call& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_function at "
                             "this stage in the compilation (after inlining).");
}

r_expr sym_diff(const resolved_constant& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_constant at "
                             "this stage in the compilation (after optimization and inlining).");
}

r_expr sym_diff(const resolved_object& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_object "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_parameter& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_parameter "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_state& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_state "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_bind& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_bind "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_initial& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_initial "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_on_event& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_on_event "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_evolve& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_evolve "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_effect& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_effect "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_export& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_export "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_let& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_let "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_conditional& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_conditional "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_variable& e, const std::string& state, const std::optional<std::string>& field) {
    return sym_diff(e.value, state, field);
}

r_expr sym_diff(const resolved_argument& e, const std::string& state, const std::optional<std::string>& field) {
    if (!field && e.name == state) {
        return make_rexpr<resolved_int>(1, e.type, e.loc);
    }
    return make_rexpr<resolved_int>(0, e.type, e.loc);
}

r_expr sym_diff(const resolved_float& e, const std::string& state, const std::optional<std::string>& field) {
    return make_rexpr<resolved_int>(0, e.type, e.loc);
}

r_expr sym_diff(const resolved_int& e, const std::string& state, const std::optional<std::string>& field) {
    return make_rexpr<resolved_int>(0, e.type, e.loc);
}

r_expr sym_diff(const resolved_unary& e, const std::string& state, const std::optional<std::string>& field) {
    switch(e.op) {
        case unary_op::exp: {
            return make_rexpr<resolved_binary>(binary_op::mul, sym_diff(e.arg, state, field), make_rexpr<resolved_unary>(e), e.loc);
        }
        case unary_op::log: {
            return make_rexpr<resolved_binary>(binary_op::div, sym_diff(e.arg, state, field), e.arg, e.loc);
        }
        case unary_op::cos: {
            auto minus_u_prime = make_rexpr<resolved_unary>(unary_op::neg, sym_diff(e.arg, state, field), e.loc);
            auto sin_u = make_rexpr<resolved_unary>(unary_op::sin, e.arg, e.loc);
            return make_rexpr<resolved_binary>(binary_op::mul, minus_u_prime, sin_u, e.loc);
        }
        case unary_op::sin: {
            auto cos_u = make_rexpr<resolved_unary>(unary_op::cos, e.arg, e.loc);
            return make_rexpr<resolved_binary>(binary_op::mul, sym_diff(e.arg, state, field), cos_u, e.loc);
        }
        case unary_op::neg: {
            auto sd = sym_diff(e.arg, state, field);
            return make_rexpr<resolved_unary>(unary_op::neg, sd, e.loc);
        }
        case unary_op::exprelr: {
            // translate to x/(e^x -1)
            auto exp_arg = make_rexpr<resolved_unary>(unary_op::exp, e.arg, e.type, e.loc);
            auto one     = make_rexpr<resolved_int>(1, e.type, e.loc);
            auto sub_one = make_rexpr<resolved_binary>(binary_op::sub, e.arg, one, e.type, e.loc);
            auto div     = make_rexpr<resolved_binary>(binary_op::div, e.arg, sub_one, e.loc);
            return sym_diff(div, state, field);
        }
        default: {
            throw std::runtime_error(fmt::format("Internal compiler error, operator {} can't be differentiated.", to_string(e.op)));
        }
    }
}

r_expr sym_diff(const resolved_binary& e, const std::string& state, const std::optional<std::string>& field) {
    switch(e.op) {
        case binary_op::add:
        case binary_op::sub: {
            auto dlhs = sym_diff(e.lhs, state, field);
            auto drhs = sym_diff(e.rhs, state, field);
            return make_rexpr<resolved_binary>(e.op, dlhs, drhs, type_of(dlhs), e.loc);
        }
        case binary_op::mul: {
            auto dlhs = sym_diff(e.lhs, state, field);
            auto drhs = sym_diff(e.rhs, state, field);


            auto u_prime_v = make_rexpr<resolved_binary>(binary_op::mul, dlhs, e.rhs, e.loc);
            auto v_prime_u = make_rexpr<resolved_binary>(binary_op::mul, e.lhs, drhs, e.loc);
            return make_rexpr<resolved_binary>(binary_op::add, u_prime_v, v_prime_u, e.loc);
        }
        case binary_op::div: {
            // u'/v   - (u/v^2 * v')
            // l_term - r_term
            auto l_term        = make_rexpr<resolved_binary>(binary_op::div, sym_diff(e.lhs, state, field), e.rhs, e.loc);
            auto v_sq          = make_rexpr<resolved_binary>(binary_op::mul, e.rhs, e.rhs, e.loc);
            auto u_div_v_sq    = make_rexpr<resolved_binary>(binary_op::div, e.lhs, v_sq, e.loc);
            auto r_term        = make_rexpr<resolved_binary>(binary_op::mul,u_div_v_sq, sym_diff(e.rhs, state, field), e.loc);
            return make_rexpr<resolved_binary>(binary_op::sub, l_term, r_term, e.loc);
        }
        case binary_op::pow: {
            // Only works if the rhs is an int
            // Or id neither lhs nor rhs is a function of the derived value
            auto lhs_prime = sym_diff(e.lhs, state, field);
            auto rhs = as_number(e.rhs);
            if (!rhs) {
                auto rhs_prime = sym_diff(e.lhs, state, field);
                auto lhs_num = as_number(constant_fold(lhs_prime).first);
                auto rhs_num = as_number(constant_fold(rhs_prime).first);
                if (!lhs_num || !rhs_num || (lhs_num.value() != 0) || (rhs_num.value() != 0)) {
                    throw std::runtime_error(fmt::format("Internal compiler error, operator {} can't be differentiated.",
                                                         to_string(e.op)));
                }
                return make_rexpr<resolved_int>(0, e.type, e.loc);
            }

            auto n_sub_1 = make_rexpr<resolved_int>(rhs.value()-1, type_of(e.rhs), location_of(e.rhs));
            auto u_pow_n_sub_1 = make_rexpr<resolved_binary>(binary_op::pow, e.lhs, n_sub_1, e.type, e.loc);
            auto n_mul_u_pow_n_sub_1 = make_rexpr<resolved_binary>(binary_op::mul, e.rhs, u_pow_n_sub_1, e.loc);
            return make_rexpr<resolved_binary>(binary_op::mul, n_mul_u_pow_n_sub_1, lhs_prime, e.loc);
        }
        default: {
            throw std::runtime_error(fmt::format("Internal compiler error, operator {} can't be differentiated.",
                                                 to_string(e.op)));
        }
    }
}

r_expr sym_diff(const resolved_field_access& e, const std::string& state, const std::optional<std::string>& field) {
    if (!field) {
        return make_rexpr<resolved_int>(0, e.type, e.loc);
    }

    if (auto arg = std::get_if<resolved_argument>(e.object.get())) {
        if (arg->name == state && e.field == field.value()) {
            return make_rexpr<resolved_int>(1, e.type, e.loc);
        }
        return make_rexpr<resolved_int>(0, e.type, e.loc);
    }

    throw std::runtime_error(fmt::format("Internal compiler error, expected resolved_argument representing a "
                                             "state before the dot at {}", to_string(e.loc)));
}

r_expr sym_diff(const r_expr& e, const std::string& state, const std::optional<std::string>& field) {
    return std::visit([&](auto& c) {return sym_diff(c, state, field);}, *e);
}

} // namespace resolved_ir
} // namespace al