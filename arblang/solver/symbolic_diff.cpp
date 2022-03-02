#include <arblang/resolver/resolved_expressions.hpp>
#include <arblang/solver/symbolic_diff.hpp>

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

r_expr sym_diff(const resolved_object& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_function at "
                             "this stage in the compilation (after inlining and optimization).");
}

r_expr sym_diff(const resolved_parameter& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_parameter "
                             "during symbolic differentiation.");
}

r_expr sym_diff(const resolved_constant& e, const std::string& state, const std::optional<std::string>& field) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_constant "
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
            return make_rexpr<resolved_binary>(binary_op::mul, sym_diff(e.arg, state, field), make_rexpr<resolved_unary>(e), e.type, e.loc);
        }
        case unary_op::log: {
            return make_rexpr<resolved_binary>(binary_op::div, sym_diff(e.arg, state, field), e.arg, e.type, e.loc);
        }
        case unary_op::cos: {
            auto minus_u_prime = make_rexpr<resolved_unary>(unary_op::neg, sym_diff(e.arg, state, field), e.type, e.loc);
            auto sin_u = make_rexpr<resolved_unary>(unary_op::sin, e.arg, e.type, e.loc);
            return make_rexpr<resolved_binary>(binary_op::mul, minus_u_prime, sin_u, e.type, e.loc);
        }
        case unary_op::sin: {
            auto cos_u = make_rexpr<resolved_unary>(unary_op::cos, e.arg, e.type, e.loc);
            return make_rexpr<resolved_binary>(binary_op::mul, sym_diff(e.arg, state, field), cos_u, e.type, e.loc);
        }
        case unary_op::neg: {
            return make_rexpr<resolved_unary>(unary_op::neg, sym_diff(e.arg, state, field), e.type, e.loc);
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
            return make_rexpr<resolved_binary>(e.op, sym_diff(e.lhs, state, field), sym_diff(e.rhs, state, field), e.type, e.loc);
        }
        case binary_op::mul: {
            auto u_prime_v = make_rexpr<resolved_binary>(binary_op::mul, sym_diff(e.lhs, state, field), e.rhs, e.type, e.loc);
            auto v_prime_u = make_rexpr<resolved_binary>(binary_op::mul, e.lhs, sym_diff(e.rhs, state, field), e.type, e.loc);
            return make_rexpr<resolved_binary>(binary_op::add, u_prime_v, v_prime_u, e.type, e.loc);
        }
        case binary_op::div: {
            auto u_prime_v = make_rexpr<resolved_binary>(binary_op::mul, sym_diff(e.lhs, state, field), e.rhs, e.type, e.loc);
            auto v_prime_u = make_rexpr<resolved_binary>(binary_op::mul, e.lhs, sym_diff(e.rhs, state, field), e.type, e.loc);
            auto numerator = make_rexpr<resolved_binary>(binary_op::sub, u_prime_v, v_prime_u, e.type, e.loc);
            auto denominator = make_rexpr<resolved_binary>(binary_op::mul, e.rhs, e.rhs, e.type, e.loc);
            return make_rexpr<resolved_binary>(binary_op::div, numerator, denominator, e.type, e.loc);
        }
        default: {
            throw std::runtime_error(fmt::format("Internal compiler error, operator {} can't be differentiated.", to_string(e.op)));
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