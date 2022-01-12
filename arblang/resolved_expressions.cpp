#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

#define FMT_HEADER_ONLY YES
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/compile.h>

#include <arblang/common.hpp>
#include <arblang/raw_expressions.hpp>
#include <arblang/resolved_expressions.hpp>
#include <arblang/resolved_types.hpp>

namespace al {
namespace resolved_ir {
using namespace raw_ir;
using namespace t_resolved_ir;

// Resolver

void check_duplicate(const std::string& name, const in_scope_map& map) {
    if (map.param_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate parameter name, also found at {}",
                                             to_string(map.param_map.at(name).loc)));
    }
    if (map.const_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate parameter name, also found at {}",
                                             to_string(map.const_map.at(name).loc)));
    }
    if (map.bind_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate parameter name, also found at {}",
                                             to_string(map.bind_map.at(name).loc)));
    }
    if (map.state_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate parameter name, also found at {}",
                                             to_string(map.state_map.at(name).loc)));
    }
}

r_expr resolve(const parameter_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto p_name = id.name;
    check_duplicate(p_name, map);

    auto available_map = map;
    available_map.bind_map.clear();
    available_map.state_map.clear();

    auto p_val = std::visit([&](auto&& c){return resolve(c, available_map);}, *(expr.value));
    auto p_type = std::visit([](auto&& c){return c.type;}, *p_val);

    if (id.type) {
        auto id_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *id.type.value());
        if (*id_type != *p_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(*id_type), to_string(*p_type), to_string(id.loc)));
        }
    }
    return make_rexpr<resolved_parameter>(p_name, p_val, p_type, expr.loc);
}

r_expr resolve(const constant_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto c_name = id.name;
    check_duplicate(c_name, map);

    auto available_map = map;
    available_map.param_map.clear();
    available_map.bind_map.clear();
    available_map.state_map.clear();

    auto c_val  = std::visit([&](auto&& c){return resolve(c, available_map);}, *(expr.value));
    auto c_type = std::visit([](auto&& c){return c.type;}, *c_val);

    if (id.type) {
        auto id_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *id.type.value());
        if (*id_type != *c_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(*id_type), to_string(*c_type), to_string(id.loc)));
        }
    }

    return make_rexpr<resolved_constant>(c_name, c_val, c_type, expr.loc);
}

r_expr resolve(const state_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto s_name = id.name;
    check_duplicate(s_name, map);

    auto s_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *id.type.value());

    if (!id.type) {
        throw std::runtime_error(fmt::format("state identifier {} at {} missing quantity type.",
                                             s_name, to_string(id.loc)));
    }
    return make_rexpr<resolved_state>(s_name, s_type, expr.loc);
}

r_expr resolve(const bind_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto b_name = id.name;
    check_duplicate(b_name, map);

    auto b_type = resolve_type_of(expr.bind, expr.loc);

    if (id.type) {
        auto id_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *id.type.value());
        if (*id_type != *b_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(*id_type), to_string(*b_type), to_string(id.loc)));
        }
    }
    return make_rexpr<resolved_bind>(b_name, expr.bind, expr.ion, b_type, expr.loc);
}

r_expr resolve(const record_alias_expr& expr, const in_scope_map& map) {
    if (map.func_map.count(expr.name)) {
        throw std::runtime_error(fmt::format("duplicate record alias name, also found at {}",
                                             to_string(map.param_map.at(expr.name).loc)));
    }
    auto a_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *expr.type);
    return make_rexpr<resolved_record_alias>(expr.name, a_type, expr.loc);
}

r_expr resolve(const function_expr& expr, const in_scope_map& map) {
    auto f_name = expr.name;
    if (map.func_map.count(f_name)) {
        throw std::runtime_error(fmt::format("duplicate function name, also found at {}",
                                             to_string(map.param_map.at(f_name).loc)));
    }
    auto available_map = map;
    std::vector<r_expr> f_args;
    for (const auto& a: expr.args) {
        auto a_id = std::get<identifier_expr>(*a);
        if (!a_id.type) {
            throw std::runtime_error(fmt::format("function argument {} at {} missing quantity type.",
                                                 a_id.name, to_string(a_id.loc)));
        }
        auto a_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *a_id.type.value());
        auto f_a = resolved_argument(a_id.name, a_type, a_id.loc);
        f_args.push_back(make_rexpr<resolved_argument>(f_a));
        available_map.local_map.insert({a_id.name, f_a});
    }
    auto f_body = std::visit([&](auto&& c){return resolve(c, available_map);}, *expr.body);
    auto f_type = std::visit([](auto&& c){return c.type;}, *f_body);

    if (expr.ret) {
        auto ret_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *expr.ret.value());
        if (*ret_type != *f_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(*ret_type), to_string(*f_type), to_string(expr.loc)));
        }
    }
    return make_rexpr<resolved_function>(f_name, f_args, f_body, f_type, expr.loc);
}

r_expr resolve(const initial_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto i_name = id.name;

    if (!map.state_map.count(i_name)) {
        throw std::runtime_error(fmt::format("variable {} initialized at {} is not a state variable.", i_name, to_string(expr.loc)));
    }
    auto i_expr = make_rexpr<resolved_state>(map.state_map.at(i_name));
    auto i_val = std::visit([&](auto&& c){return resolve(c, map);}, *(expr.value));
    auto i_type = std::visit([](auto&& c){return c.type;}, *i_val);

    if (id.type) {
        auto id_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *id.type.value());
        if (*id_type != *i_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(*id_type), to_string(*i_type), to_string(id.loc)));
        }
    }
    return make_rexpr<resolved_initial>(i_expr, i_val, i_type, expr.loc);
}

r_expr resolve(const evolve_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto e_name = id.name;
    if (e_name.back() != '\'') {
        throw std::runtime_error(fmt::format("variable {} evolved at {} is not a derivative.", e_name, to_string(expr.loc)));
    }
    e_name.pop_back();
    if (!map.state_map.count(e_name)) {
        throw std::runtime_error(fmt::format("variable {} evolved at {} is not a state variable.", e_name, to_string(expr.loc)));
    }
    auto s_expr = map.state_map.at(e_name);
    auto e_expr = make_rexpr<resolved_state>(s_expr);
    auto e_val = std::visit([&](auto&& c){return resolve(c, map);}, *(expr.value));
    auto e_type = std::visit([](auto&& c){return c.type;}, *e_val);

    if (id.type) {
        auto id_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *id.type.value());
        if (*id_type != *e_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(*id_type), to_string(*e_type), to_string(id.loc)));
        }
    }
    return make_rexpr<resolved_evolve>(e_expr, e_val, e_type, expr.loc);
}

r_expr resolve(const effect_expr& expr, const in_scope_map& map) {
    auto e_effect = expr.effect;
    auto e_ion = expr.ion;
    auto e_val = std::visit([&](auto&& c){return resolve(c, map);}, *(expr.value));
    auto e_type = resolve_type_of(e_effect, expr.loc);

    return make_rexpr<resolved_effect>(e_effect, e_ion, e_val, e_type, expr.loc);
}

r_expr resolve(const export_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto p_name = id.name;

    if (!map.param_map.count(p_name)) {
        throw std::runtime_error(fmt::format("variable {} exported at {} is not a parameter.", p_name, to_string(expr.loc)));
    }
    auto p_expr = map.param_map.at(p_name);
    return make_rexpr<resolved_export>(make_rexpr<resolved_parameter>(p_expr), p_expr.type, expr.loc);
}

r_expr resolve(const call_expr& expr, const in_scope_map& map) {
    auto f_name = expr.function_name;
    if (!map.func_map.count(f_name)) {
        throw std::runtime_error(fmt::format("function {} called at {} is not defined.", f_name, to_string(expr.loc)));
    }
    auto f_expr = map.func_map.at(f_name);

    std::vector<r_expr> c_args;
    for (const auto& a: expr.call_args) {
        c_args.push_back(std::visit([&](auto&& c){return resolve(c, map);}, *a));
    }
    if (f_expr.args.size() != c_args.size()) {
        throw std::runtime_error(fmt::format("argument count mismatch when calling function {} at {}.", f_name, to_string(expr.loc)));
    }
    for (unsigned i=0; i < f_expr.args.size(); ++i) {
        auto f_arg_type = std::visit([](auto&& c){return c.type;}, *f_expr.args[i]);
        auto c_arg_type = std::visit([](auto&& c){return c.type;}, *f_expr.args[i]);
        if (*f_arg_type != *c_arg_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} of argument {} of function call {} at {}.",
                                     to_string(*f_arg_type), to_string(*c_arg_type), std::to_string(i), f_name,to_string(expr.loc)));
        }
    }

    return make_rexpr<resolved_call>(make_rexpr<resolved_function>(f_expr), c_args, f_expr.type, expr.loc);
}

r_expr resolve(const object_expr& expr, const in_scope_map& map) {
    assert(expr.record_fields.size() == expr.record_values.size());
    std::vector<r_expr> o_values;
    std::vector<r_expr> o_fields;
    std::vector<r_type> o_types;

    for (const auto& v: expr.record_values) {
        o_values.push_back(std::visit([&](auto&& c){return resolve(c, map);}, *v));
        o_types.push_back(std::visit([&](auto&& c){return c.type;}, *o_values.back()));
    }
    std::vector<std::pair<std::string, r_type>> t_vec;
    for (unsigned i = 0; i < expr.record_fields.size(); ++i) {
        auto f_id = std::get<identifier_expr>(*expr.record_fields[i]);
        o_fields.push_back(make_rexpr<resolved_argument>(f_id.name, o_types[i], f_id.loc));
        t_vec.emplace_back(f_id.name, o_types[i]);
    }
    auto o_type = make_rtype<resolved_record>(t_vec, expr.loc);

    if (!expr.record_name) {
        return make_rexpr<resolved_object>(std::nullopt, o_fields, o_values, o_type, expr.loc);
    }

    auto r_name = expr.record_name.value();
    if (!map.type_map.count(r_name)) {
        throw std::runtime_error(fmt::format("record {} referenced at {} is not defined.", r_name, to_string(expr.loc)));
    }
    auto r_type = map.type_map.at(r_name);
    if (*r_type != *o_type) {
        throw std::runtime_error(fmt::format("type mismatch between {} and {} while constructing object {} at {}.",
                                             to_string(*r_type), to_string(*o_type), r_name, to_string(expr.loc)));
    }

    return make_rexpr<resolved_object>(std::nullopt, o_fields, o_values, o_type, expr.loc);
}

r_expr resolve(const let_expr& expr, const in_scope_map& map) {
    auto v_expr = std::visit([&](auto&& c){return resolve(c, map);}, *expr.value);
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto i_type = std::visit([&](auto&& c){return c.type;}, *v_expr);
    if (id.type) {
        auto id_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *id.type.value());
        if (*id_type != *i_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(*id_type), to_string(*i_type), to_string(id.loc)));
        }
    }
    auto i_expr = resolved_argument(id.name, i_type, id.loc);

    auto available_map = map;
    available_map.local_map.insert({id.name, i_expr});

    auto b_expr = std::visit([&](auto&& c){return resolve(c, available_map);}, *expr.body);
    auto b_type = std::visit([&](auto&& c){return c.type;}, *b_expr);

    return make_rexpr<resolved_let>(make_rexpr<resolved_argument>(i_expr), v_expr, b_expr, b_type, expr.loc);
}

r_expr resolve(const with_expr& expr, const in_scope_map& map) {
    auto v_expr = std::visit([&](auto&& c){return resolve(c, map);}, *expr.value);
    auto v_type = std::visit([&](auto&& c){return c.type;}, *v_expr);

    auto available_map = map;
    if (std::get_if<resolved_record>(v_type.get())) {
        auto v_rec = std::get<resolved_object>(*v_expr);
        for (const auto& v_field: v_rec.record_fields) {
            auto v_arg = std::get<resolved_argument>(*v_field);
            available_map.local_map.insert({v_arg.name, v_arg});
        }
    }
    else {
        auto v_string = std::visit([](auto&& c){return to_string(c);}, *expr.value);
        throw std::runtime_error(fmt::format("with value {} referenced at {} is not a record type.", v_string, to_string(expr.loc)));
    }

    auto b_expr = std::visit([&](auto&& c){return resolve(c, available_map);}, *expr.body);
    auto b_type = std::visit([&](auto&& c){return c.type;}, *b_expr);

    return make_rexpr<resolved_with>(v_expr, b_expr, b_type, expr.loc);
}

r_expr resolve(const conditional_expr& expr, const in_scope_map& map) {
    auto cond    = std::visit([&](auto&& c){return resolve(c, map);}, *expr.condition);
    auto true_v  = std::visit([&](auto&& c){return resolve(c, map);}, *expr.value_true);
    auto false_v = std::visit([&](auto&& c){return resolve(c, map);}, *expr.value_false);
    auto true_t  = std::visit([&](auto&& c){return c.type;}, *true_v);
    auto false_t = std::visit([&](auto&& c){return c.type;}, *false_v);

    if (*true_t != *false_t) {
        throw std::runtime_error(fmt::format("type mismatch {} and {} between the true and false branches "
                                             "of the conditional statement at {}.",
                                             to_string(*true_t), to_string(*false_t), to_string(expr.loc)));
    }

    return make_rexpr<resolved_conditional>(cond, true_v, false_v, true_t, expr.loc);
}

r_expr resolve(const float_expr& expr, const in_scope_map& map) {
    using namespace u_raw_ir;
    auto f_val = expr.value;
    auto f_type = std::visit([&](auto&& c){return to_type(c);}, *expr.unit);
    auto r_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *f_type);
    return make_rexpr<resolved_float>(f_val, r_type, expr.loc);
}

r_expr resolve(const int_expr& expr, const in_scope_map& map) {
    using namespace u_raw_ir;
    auto i_val = expr.value;
    auto i_type = std::visit([&](auto&& c){return to_type(c);}, *expr.unit);
    auto r_type = std::visit([&](auto&& c){return resolve_type_of(c, map.type_map);}, *i_type);
    return make_rexpr<resolved_int>(i_val, r_type, expr.loc);
}

r_expr resolve(const unary_expr& expr, const in_scope_map& map) {
    auto val = std::visit([&](auto&& c){return resolve(c, map);}, *expr.value);
    auto type = std::visit([&](auto&& c){return c.type;}, *val);
    switch (expr.op) {
        case unary_op::exp:
        case unary_op::log:
        case unary_op::cos:
        case unary_op::sin:
        case unary_op::exprelr: {
            auto q_type = std::get_if<resolved_quantity>(type.get());
            if (!q_type) {
                throw std::runtime_error(fmt::format("Cannot apply op {} to non-real type, at {}",
                                                     to_string(expr.op), to_string(expr.loc)));
            }
            if (!q_type->type.is_real()) {
                throw std::runtime_error(fmt::format("Cannot apply op {} to non-real type, at {}",
                                                     to_string(expr.op), to_string(expr.loc)));
            }
            break;
        }
        case unary_op::lnot: {
            if (!std::get_if<resolved_boolean>(type.get())) {
                throw std::runtime_error(fmt::format("Cannot apply op {} to non-boolean type, at {}",
                                                     to_string(expr.op), to_string(expr.loc)));
            }
            break;
        }
        case unary_op::neg: {
            if (std::get_if<resolved_record>(type.get())) {
                throw std::runtime_error(fmt::format("Cannot apply op {} to record type, at {}",
                                                     to_string(expr.op), to_string(expr.loc)));
            }
            break;
        }
        default: break;
    }
    return make_rexpr<resolved_unary>(expr.op, val, type, expr.loc);
}

r_expr resolve(const binary_expr& expr, const in_scope_map& map) {
    auto lhs_v = std::visit([&](auto&& c){return resolve(c, map);}, *expr.lhs);
    auto lhs_t = std::visit([&](auto&& c){return c.type;}, *lhs_v);
    auto lhs_loc = std::visit([&](auto&& c){return c.loc;}, *lhs_v);

    if (expr.op == binary_op::dot) {
        auto make_or_throw = [&](const resolved_record& r) {
            // Add fields to the local map
            auto available_map = map;
            for (auto [f_id, f_type]: r.fields) {
                available_map.local_map.insert({f_id, resolved_argument(f_id, f_type, r.loc)});
            }
            auto rhs_v = std::visit([&](auto&& c){return resolve(c, available_map);}, *expr.rhs);

            // Resolved rhs is expected to be one of the just added fields
            // But we need to check

            // Check that it is a resolved_argument
            auto* r_rhs = std::get_if<resolved_argument>(rhs_v.get());
            if (!r_rhs) {
                throw std::runtime_error(fmt::format("incompatible argument type to dot operator, at {}", to_string(expr.loc)));
            }
            auto rhs_id = (*r_rhs).name;

            // Check that it is an argument of the record
            for (auto [f_id, f_type]: r.fields) {
                if (rhs_id == f_id) {
                    return make_rexpr<resolved_binary>(expr.op, lhs_v, rhs_v, f_type, expr.loc);
                }
            }
            throw std::runtime_error(fmt::format("argument {} doesn't match any of the record fields, at {}",
                                                 to_string(*r_rhs), to_string(lhs_loc)));
        };

        if (auto* lhs_rec = std::get_if<resolved_record>(lhs_t.get())) {
            return make_or_throw(*lhs_rec);
        }
    }

    // If it's not a field access, we can use the input map to resolve rhs
    auto rhs_v = std::visit([&](auto&& c){return resolve(c, map);}, *expr.rhs);
    auto rhs_t = std::visit([&](auto&& c){return c.type;}, *rhs_v);
    auto rhs_loc = std::visit([&](auto&& c){return c.loc;}, *rhs_v);

    // In this case, neither lhs nor rhs can have record types.
    if (std::get_if<resolved_record>(rhs_t.get())) {
        throw std::runtime_error(fmt::format("Cannot apply op {} to record type, at {}",
                                             to_string(expr.op), to_string(rhs_loc)));
    }
    if (std::get_if<resolved_record>(lhs_t.get())) {
        throw std::runtime_error(fmt::format("Cannot apply op {} to record type, at {}",
                                             to_string(expr.op), to_string(lhs_loc)));
    }

    // And, they both have to have the same type.
    if ((std::get_if<resolved_boolean>(lhs_t.get()) && !std::get_if<resolved_boolean>(rhs_t.get())) ||
        (!std::get_if<resolved_boolean>(lhs_t.get()) && std::get_if<resolved_boolean>(rhs_t.get()))) {
        throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                             to_string(expr.op), to_string(expr.loc)));
    }

    auto lhs_q = std::get_if<resolved_quantity>(lhs_t.get());
    auto rhs_q = std::get_if<resolved_quantity>(rhs_t.get());
    switch (expr.op) {
        case binary_op::min:
        case binary_op::max:
        case binary_op::add:
        case binary_op::sub: {
            // Only applicable if neither lhs or rhs is a boolean type
            if (lhs_q && rhs_q) {
                if (lhs_q->type != rhs_q->type) {
                    throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                         to_string(expr.op), to_string(expr.loc)));
                }
                return make_rexpr<resolved_binary>(expr.op, lhs_v, rhs_v, lhs_t, expr.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                   to_string(expr.op), to_string(expr.loc)));
        }
        case binary_op::mul: {
            // Only applicable if neither lhs or rhs is a boolean type
            if (lhs_q && rhs_q) {
                auto comb_type = make_rtype<resolved_quantity>(lhs_q->type*rhs_q->type, expr.loc);
                return make_rexpr<resolved_binary>(expr.op, lhs_v, rhs_v, comb_type, expr.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                 to_string(expr.op), to_string(expr.loc)));
        }
        case binary_op::div: {
            // Only applicable if neither lhs or rhs is a boolean type
            if (lhs_q && rhs_q) {
                auto comb_type = make_rtype<resolved_quantity>(lhs_q->type/rhs_q->type, expr.loc);
                return make_rexpr<resolved_binary>(expr.op, lhs_v, rhs_v, comb_type, expr.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                 to_string(expr.op), to_string(expr.loc)));
        }
        case binary_op::pow: {
            // Only applicable if neither lhs or rhs is a boolean type and rhs is a real int type
            auto rhs_int = std::get_if<resolved_int>(rhs_v.get());
            if (!rhs_int) {
                throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                     to_string(expr.op), to_string(expr.loc)));
            }
            auto rhs_int_q = std::get_if<resolved_quantity>(rhs_int->type.get());
            if (!rhs_int_q || !rhs_int_q->type.is_real()) {
                throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                     to_string(expr.op), to_string(expr.loc)));
            }
            if (lhs_q && rhs_q) {
                auto comb_type = make_rtype<resolved_quantity>(lhs_q->type^rhs_int->value, expr.loc);
                return make_rexpr<resolved_binary>(expr.op, lhs_v, rhs_v, comb_type, expr.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                 to_string(expr.op), to_string(expr.loc)));
        }
        case binary_op::lt:
        case binary_op::le:
        case binary_op::gt:
        case binary_op::ge:
        case binary_op::eq:
        case binary_op::ne: {
            // Only applicable if neither lhs or rhs is a boolean type
            if (lhs_q && rhs_q) {
                if (lhs_q->type != rhs_q->type) {
                    throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                         to_string(expr.op), to_string(expr.loc)));
                }
            }
            return make_rexpr<resolved_binary>(expr.op, lhs_v, rhs_v, make_rtype<resolved_boolean>(expr.loc), expr.loc);
        }
        case binary_op::land:
        case binary_op::lor: {
            return make_rexpr<resolved_binary>(expr.op, lhs_v, rhs_v, make_rtype<resolved_boolean>(expr.loc), expr.loc);
        }
        default: break;
    }
    return make_rexpr<resolved_binary>(expr.op, lhs_v, rhs_v, lhs_t, expr.loc);
}

r_expr resolve(const identifier_expr& expr, const in_scope_map& map) {
    if (map.param_map.count(expr.name)) {
        return make_rexpr<resolved_parameter>(map.param_map.at(expr.name));
    }
    if (map.const_map.count(expr.name)) {
        return make_rexpr<resolved_constant>(map.const_map.at(expr.name));
    }
    if (map.bind_map.count(expr.name)) {
        return make_rexpr<resolved_bind>(map.bind_map.at(expr.name));
    }
    if (map.state_map.count(expr.name)) {
        return make_rexpr<resolved_state>(map.state_map.at(expr.name));
    }
    if (map.local_map.count(expr.name)) {
        return make_rexpr<resolved_argument>(map.local_map.at(expr.name));
    }
    throw std::runtime_error(fmt::format("undefined identifier {}, at {}",
                                         expr.name, to_string(expr.loc)));
}

resolved_mechanism resolve(const mechanism_expr& expr, const in_scope_map& map) {
    resolved_mechanism mech;
    auto available_map = map;
    for (const auto& c: expr.constants) {
        auto val = std::visit([&](auto&& c){return resolve(c, available_map);}, *c);
        mech.constants.push_back(val);
        auto const_val = std::get_if<resolved_constant>(val.get());
        if (!const_val) {
            auto loc = std::visit([&](auto&& c){return c.loc;}, *val);
            throw std::runtime_error(fmt::format("internal compiler error, expected constant expression at {}", to_string(loc)));
        }
        available_map.const_map.insert({const_val->name, *const_val});
    }
    for (const auto& c: expr.parameters) {
        auto val = std::visit([&](auto&& c){return resolve(c, available_map);}, *c);
        mech.parameters.push_back(val);
        auto param_val = std::get_if<resolved_parameter>(val.get());
        if (!param_val) {
            auto loc = std::visit([&](auto&& c){return c.loc;}, *val);
            throw std::runtime_error(fmt::format("internal compiler error, expected parameter expression at {}", to_string(loc)));
        }
        available_map.param_map.insert({param_val->name, *param_val});
    }
    for (const auto& c: expr.bindings) {
        auto val = std::visit([&](auto&& c){return resolve(c, available_map);}, *c);
        mech.bindings.push_back(val);
        auto bind_val = std::get_if<resolved_bind>(val.get());
        if (!bind_val) {
            auto loc = std::visit([&](auto&& c){return c.loc;}, *val);
            throw std::runtime_error(fmt::format("internal compiler error, expected bind expression at {}", to_string(loc)));
        }
        available_map.bind_map.insert({bind_val->name, *bind_val});
    }
    // For records, create 2 records: regular and prime
    for (const auto& r: expr.records) {
        auto loc = std::visit([&](auto&& c){return c.loc;}, *r);
        auto rec = std::get_if<record_alias_expr>(r.get());
        if (!rec) {
            throw std::runtime_error(fmt::format("internal compiler error, expected record expression at {}", to_string(loc)));
        }

        // regular
        auto resolved_record = std::visit([&](auto&& c){return resolve(c, available_map);}, *r);
        auto resolved_record_val = std::get_if<resolved_record_alias>(resolved_record.get());
        if (!resolved_record_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected record expression at {}", to_string(loc)));
        }
        available_map.type_map.insert({resolved_record_val->name, resolved_record_val->type});

        // prime
        auto derived_record_type = std::visit([](auto&& c){return derive(c);}, *resolved_record_val->type);
        if (derived_record_type) {
            available_map.type_map.insert({resolved_record_val->name+"'", derived_record_type.value()});
        }
    }
    for (const auto& c: expr.states) {
        auto val = std::visit([&](auto&& c){return resolve(c, available_map);}, *c);
        mech.states.push_back(val);
        auto state_val = std::get_if<resolved_state>(val.get());
        if (!state_val) {
            auto loc = std::visit([&](auto&& c){return c.loc;}, *val);
            throw std::runtime_error(fmt::format("internal compiler error, expected state expression at {}", to_string(loc)));
        }
        available_map.state_map.insert({state_val->name, *state_val});
    }
    for (const auto& c: expr.functions) {
        auto val = std::visit([&](auto&& c){return resolve(c, available_map);}, *c);
        mech.functions.push_back(val);
        auto func_val = std::get_if<resolved_function>(val.get());
        if (!func_val) {
            auto loc = std::visit([&](auto&& c){return c.loc;}, *val);
            throw std::runtime_error(fmt::format("internal compiler error, expected funcyion expression at {}", to_string(loc)));
        }
        available_map.func_map.insert({func_val->name, *func_val});
    }
    for (const auto& c: expr.initializations) {
        auto val = std::visit([&](auto&& c){return resolve(c, available_map);}, *c);
        mech.initializations.push_back(val);
    }
    for (const auto& c: expr.evolutions) {
        auto val = std::visit([&](auto&& c){return resolve(c, available_map);}, *c);
        mech.evolutions.push_back(val);
    }
    for (const auto& c: expr.effects) {
        auto val = std::visit([&](auto&& c){return resolve(c, available_map);}, *c);
        mech.effects.push_back(val);
    }
    for (const auto& c: expr.exports) {
        auto val = std::visit([&](auto&& c){return resolve(c, available_map);}, *c);
        mech.exports.push_back(val);
    }
    mech.name = expr.name;
    mech.loc = expr.loc;
    mech.kind = expr.kind;
    return mech;
}

// resolved_mechanism
std::string to_string(const resolved_mechanism& e, int indent) {
    auto indent_str = std::string(indent*2, ' ');
    std::string str = indent_str + "(module_expr " + e.name + " " + to_string(e.kind) + "\n";
    for (const auto& p: e.parameters) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.constants) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.states) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.bindings) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.functions) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.initializations) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.evolutions) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.effects) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.exports) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    return str + to_string(e.loc) + ")";
}

// resolved_parameter
std::string to_string(const resolved_parameter& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_parameter\n";
    str += (double_indent + e.name + "\n");
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_constant
std::string to_string(const resolved_constant& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_constant\n";
    str += (double_indent + e.name + "\n");
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_state
std::string to_string(const resolved_state& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_state\n";
    str += (double_indent + e.name + "\n");
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_record_alias
std::string to_string(const resolved_record_alias& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_record_alias\n";
    str += (double_indent + e.name + "\n");
    std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_function
std::string to_string(const resolved_function& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_function\n";
    str += (double_indent + e.name +  "\n");
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);

    str += (double_indent + "(\n");
    for (const auto& f: e.args) {
        std::visit([&](auto&& c){str += (to_string(c, indent+2) + "\n");}, *f);
    }
    str += (double_indent + ")\n");
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.body);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_bind
std::string to_string(const resolved_bind& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_bind\n";
    str += (double_indent + to_string(e.bind));
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    str += (double_indent + e.name + "\n");
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    str += (double_indent + to_string(e.loc) + ")");
    return str;
}

// resolved_initial
std::string to_string(const resolved_initial& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_initial\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_evolve
std::string to_string(const resolved_evolve& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_evolve\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_effect
std::string to_string(const resolved_effect& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_effect\n";
    str += (double_indent + to_string(e.effect));
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_export
std::string to_string(const resolved_export& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_export\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_call
std::string to_string(const resolved_call& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_call\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.f_identifier);
    for (const auto& f: e.call_args) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *f);
    }
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_object
std::string to_string(const resolved_object& e, int indent) {
    assert(e.record_fields.size() == e.record_values.size());

    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_object\n";
    if (e.r_identifier) std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.r_identifier.value());
    for (unsigned i = 0; i < e.record_fields.size(); ++i) {
        str += (double_indent + "(\n");
        std::visit([&](auto&& c){str += (to_string(c, indent+2) + "\n");}, *(e.record_fields[i]));
        std::visit([&](auto&& c){str += (to_string(c, indent+2) + "\n");}, *(e.record_values[i]));
        str += (double_indent + ")\n");
    }
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_let
std::string to_string(const resolved_let& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_let\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.body);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_with
std::string to_string(const resolved_with& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";
    std::string str = single_indent + "(resolved_with\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.body);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_conditional
std::string to_string(const resolved_conditional& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_conditional\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.condition);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value_true);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value_false);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_float
std::string to_string(const resolved_float& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_float\n";
    str += (double_indent + std::to_string(e.value) + "\n");
    std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_int
std::string to_string(const resolved_int& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_int\n";
    str += (double_indent + std::to_string(e.value) + "\n");
    std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_unary
std::string to_string(const resolved_unary& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_unary " + to_string(e.op) + "\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.arg);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_binary
std::string to_string(const resolved_binary& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_binary " + to_string(e.op) + "\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.lhs);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.rhs);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}

std::string to_string(const resolved_argument& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_argument \n";
    str += (double_indent + e.name + "\n");
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    return str + double_indent + to_string(e.loc) + ")";
}
} // namespace al
} // namespace raw_ir
