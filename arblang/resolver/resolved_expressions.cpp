#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

#include <fmt/core.h>

#include <arblang/parser/parsed_expressions.hpp>
#include <arblang/resolver/resolved_expressions.hpp>
#include <arblang/resolver/resolved_types.hpp>
#include <arblang/util/common.hpp>

namespace al {
namespace resolved_ir {
using namespace parsed_ir;
using namespace resolved_type_ir;

// Resolver
void check_duplicate(const std::string& name, const src_location& loc, const in_scope_map& map) {
    if (map.param_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate definition, found at {} and {}",
                                             to_string(map.param_map.at(name).loc), to_string(loc)));
    }
    if (map.const_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate constant name, also found at {}",
                                             to_string(map.const_map.at(name).loc), to_string(loc)));
    }
    if (map.bind_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate binding name, also found at {}",
                                             to_string(map.bind_map.at(name).loc), to_string(loc)));
    }
    if (map.state_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate state name, also found at {}",
                                             to_string(map.state_map.at(name).loc), to_string(loc)));
    }
}

r_expr resolve(const parsed_parameter& e, const in_scope_map& map) {
    auto id = std::get<parsed_identifier>(*e.identifier);
    auto p_name = id.name;
    check_duplicate(p_name, id.loc, map);

    auto available_map = map;
    available_map.bind_map.clear();
    available_map.state_map.clear();

    auto p_val = resolve(e.value, available_map);;
    auto p_type = type_of(p_val);

    if (id.type) {
        auto id_type = resolve_type(id.type.value(), map.type_map);
        if (*id_type != *p_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(p_type), to_string(id.loc)));
        }
    }
    return make_rexpr<resolved_parameter>(p_name, p_val, p_type, e.loc);
}

r_expr resolve(const parsed_constant& e, const in_scope_map& map) {
    auto id = std::get<parsed_identifier>(*e.identifier);
    auto c_name = id.name;
    check_duplicate(c_name, id.loc, map);

    auto available_map = map;
    available_map.param_map.clear();
    available_map.bind_map.clear();
    available_map.state_map.clear();

    auto c_val  = resolve(e.value, available_map);
    auto c_type = type_of(c_val);

    if (id.type) {
        auto id_type = resolve_type(id.type.value(), map.type_map);
        if (*id_type != *c_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(c_type), to_string(id.loc)));
        }
    }

    return make_rexpr<resolved_constant>(c_name, c_val, c_type, e.loc);
}

r_expr resolve(const parsed_state& e, const in_scope_map& map) {
    auto id = std::get<parsed_identifier>(*e.identifier);
    auto s_name = id.name;
    check_duplicate(s_name, id.loc, map);

    auto s_type = resolve_type(id.type.value(), map.type_map);

    if (!id.type) {
        throw std::runtime_error(fmt::format("state identifier {} at {} missing quantity type.",
                                             s_name, to_string(id.loc)));
    }
    return make_rexpr<resolved_state>(s_name, s_type, e.loc);
}

r_expr resolve(const parsed_bind& e, const in_scope_map& map) {
    auto id = std::get<parsed_identifier>(*e.identifier);
    auto b_name = id.name;
    check_duplicate(b_name, id.loc, map);

    auto b_type = resolve_type(e.bind, e.loc);

    if (id.type) {
        auto id_type = resolve_type(id.type.value(), map.type_map);
        if (*id_type != *b_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(b_type), to_string(id.loc)));
        }
    }
    return make_rexpr<resolved_bind>(b_name, e.bind, e.ion, b_type, e.loc);
}

r_expr resolve(const parsed_record_alias& e, const in_scope_map& map) {
    if (map.func_map.count(e.name)) {
        throw std::runtime_error(fmt::format("duplicate record alias name, also found at {}",
                                             to_string(map.param_map.at(e.name).loc)));
    }
    auto a_type = resolve_type(e.type, map.type_map);
    return make_rexpr<resolved_record_alias>(e.name, a_type, e.loc);
}

r_expr resolve(const parsed_function& e, const in_scope_map& map) {
    auto f_name = e.name;
    if (map.func_map.count(f_name)) {
        throw std::runtime_error(fmt::format("duplicate function name, also found at {}",
                                             to_string(map.param_map.at(f_name).loc)));
    }
    auto available_map = map;
    std::vector<r_expr> f_args;
    for (const auto& a: e.args) {
        auto a_id = std::get<parsed_identifier>(*a);
        if (!a_id.type) {
            throw std::runtime_error(fmt::format("function argument {} at {} missing quantity type.",
                                                 a_id.name, to_string(a_id.loc)));
        }
        auto a_type = resolve_type(a_id.type.value(), map.type_map);
        auto f_a = resolved_argument(a_id.name, a_type, a_id.loc);
        f_args.push_back(make_rexpr<resolved_argument>(f_a));
        available_map.local_map.insert_or_assign(a_id.name, f_a);
    }
    auto f_body = resolve(e.body, available_map);
    auto f_type = type_of(f_body);

    if (e.ret) {
        auto ret_type = resolve_type(e.ret.value(), map.type_map);
        if (*ret_type != *f_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(ret_type), to_string(f_type), to_string(e.loc)));
        }
    }
    return make_rexpr<resolved_function>(f_name, f_args, f_body, f_type, e.loc);
}

r_expr resolve(const parsed_initial& e, const in_scope_map& map) {
    auto id = std::get<parsed_identifier>(*e.identifier);
    auto i_name = id.name;

    if (!map.state_map.count(i_name)) {
        throw std::runtime_error(fmt::format("variable {} initialized at {} is not a state variable.", i_name, to_string(e.loc)));
    }
    auto i_expr = make_rexpr<resolved_state>(map.state_map.at(i_name));
    auto i_val = resolve(e.value, map);
    auto i_type = type_of(i_val);

    if (id.type) {
        auto id_type = resolve_type(id.type.value(), map.type_map);
        if (*id_type != *i_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(i_type), to_string(id.loc)));
        }
    }
    return make_rexpr<resolved_initial>(i_expr, i_val, i_type, e.loc);
}

r_expr resolve(const parsed_evolve& e, const in_scope_map& map) {
    auto id = std::get<parsed_identifier>(*e.identifier);
    auto e_name = id.name;
    if (e_name.back() != '\'') {
        throw std::runtime_error(fmt::format("variable {} evolved at {} is not a derivative.", e_name, to_string(e.loc)));
    }
    e_name.pop_back();
    if (!map.state_map.count(e_name)) {
        throw std::runtime_error(fmt::format("variable {} evolved at {} is not a state variable.", e_name, to_string(e.loc)));
    }
    auto s_expr = map.state_map.at(e_name);
    auto s_val = make_rexpr<resolved_state>(s_expr);
    auto s_type = derive(s_expr.type).value();
    auto e_val = resolve(e.value, map);
    auto e_type = type_of(e_val);

    if (*s_type != *e_type) {
        throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                             to_string(s_type), to_string(e_type), to_string(id.loc)));
    }

    if (id.type) {
        auto id_type = resolve_type(id.type.value(), map.type_map);
        if (*id_type != *e_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(e_type), to_string(id.loc)));
        }
    }
    return make_rexpr<resolved_evolve>(s_val, e_val, e_type, e.loc);
}

r_expr resolve(const parsed_effect& e, const in_scope_map& map) {
    auto e_effect = e.effect;
    auto e_ion = e.ion;
    auto e_val = resolve(e.value, map);
    auto e_type = resolve_type(e_effect, e.loc);

    return make_rexpr<resolved_effect>(e_effect, e_ion, e_val, e_type, e.loc);
}

r_expr resolve(const parsed_export& e, const in_scope_map& map) {
    auto id = std::get<parsed_identifier>(*e.identifier);
    auto p_name = id.name;

    if (!map.param_map.count(p_name)) {
        throw std::runtime_error(fmt::format("variable {} exported at {} is not a parameter.", p_name, to_string(e.loc)));
    }
    auto p_expr = map.param_map.at(p_name);
    return make_rexpr<resolved_export>(make_rexpr<resolved_parameter>(p_expr), p_expr.type, e.loc);
}

r_expr resolve(const parsed_call& e, const in_scope_map& map) {
    auto f_name = e.function_name;
    if (!map.func_map.count(f_name)) {
        throw std::runtime_error(fmt::format("function {} called at {} is not defined.", f_name, to_string(e.loc)));
    }
    auto f_expr = map.func_map.at(f_name);

    std::vector<r_expr> c_args;
    for (const auto& a: e.call_args) {
        c_args.push_back(resolve(a, map));
    }
    if (f_expr.args.size() != c_args.size()) {
        throw std::runtime_error(fmt::format("argument count mismatch when calling function {} at {}.", f_name, to_string(e.loc)));
    }
    for (unsigned i=0; i < f_expr.args.size(); ++i) {
        auto f_arg_type = type_of(f_expr.args[i]);
        auto c_arg_type = type_of(c_args[i]);
        if (*f_arg_type != *c_arg_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} of argument {} of function call {} at {}.",
                                     to_string(f_arg_type), to_string(c_arg_type), std::to_string(i), f_name,to_string(e.loc)));
        }
    }

    return make_rexpr<resolved_call>(make_rexpr<resolved_function>(f_expr), c_args, f_expr.type, e.loc);
}

r_expr resolve(const parsed_object& e, const in_scope_map& map) {
    assert(e.record_fields.size() == e.record_values.size());
    std::vector<r_expr> o_values;
    std::vector<r_expr> o_fields;
    std::vector<r_type> o_types;

    for (const auto& v: e.record_values) {
        o_values.push_back(resolve(v, map));
        o_types.push_back(type_of(o_values.back()));
    }
    std::vector<std::pair<std::string, r_type>> t_vec;
    for (unsigned i = 0; i < e.record_fields.size(); ++i) {
        auto f_id = std::get<parsed_identifier>(*e.record_fields[i]);
        o_fields.push_back(make_rexpr<resolved_argument>(f_id.name, o_types[i], f_id.loc));
        t_vec.emplace_back(f_id.name, o_types[i]);
    }
    auto o_type = make_rtype<resolved_record>(t_vec, e.loc);

    if (!e.record_name) {
        return make_rexpr<resolved_object>(std::nullopt, o_fields, o_values, o_type, e.loc);
    }

    auto r_name = e.record_name.value();
    if (!map.type_map.count(r_name)) {
        throw std::runtime_error(fmt::format("record {} referenced at {} is not defined.", r_name, to_string(e.loc)));
    }
    auto r_type = map.type_map.at(r_name);
    if (*r_type != *o_type) {
        throw std::runtime_error(fmt::format("type mismatch between {} and {} while constructing object {} at {}.",
                                             to_string(r_type), to_string(o_type), r_name, to_string(e.loc)));
    }

    return make_rexpr<resolved_object>(std::nullopt, o_fields, o_values, o_type, e.loc);
}

r_expr resolve(const parsed_let& e, const in_scope_map& map) {
    auto v_expr = resolve(e.value, map);
    auto id = std::get<parsed_identifier>(*e.identifier);
    check_duplicate(id.name, id.loc, map);

    auto i_type = type_of(v_expr);
    if (id.type) {
        auto id_type = resolve_type(id.type.value(), map.type_map);
        if (*id_type != *i_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(i_type), to_string(id.loc)));
        }
    }
    auto i_expr = resolved_argument(id.name, i_type, id.loc);

    auto available_map = map;
    available_map.local_map.insert_or_assign(id.name, i_expr);

    auto b_expr = resolve(e.body, available_map);
    auto b_type = type_of(b_expr);

    return make_rexpr<resolved_let>(make_rexpr<resolved_argument>(i_expr), v_expr, b_expr, b_type, e.loc);
}

r_expr resolve(const parsed_with& e, const in_scope_map& map) {
    auto v_expr = resolve(e.value, map);
    auto v_type = type_of(v_expr);

    // Transform with to nested let expressions
    std::vector<parsed_let> let_vars;
    if (auto v_rec = std::get_if<resolved_record>(v_type.get())) {
        for (const auto& v_field: v_rec->fields) {
            auto let = parsed_let();
            auto iden = make_pexpr<parsed_identifier>(v_field.first, e.loc);
            let_vars.push_back(parsed_let(iden, make_pexpr<parsed_binary>(binary_op::dot, e.value, iden, e.loc), {}, e.loc));
        }
    }
    else {
        throw std::runtime_error(fmt::format("with value {} referenced at {} is not a record type.",
                                             to_string(e.value), to_string(e.loc)));
    }

    let_vars.back().body = e.body;
    for (int i = let_vars.size()-2; i >=0; i--) {
        let_vars[i].body = make_pexpr<parsed_let>(let_vars[i+1]);
    }
    auto equivalent_let = make_pexpr<parsed_let>(let_vars.front());

    return resolve(equivalent_let, map);
}

r_expr resolve(const parsed_conditional& e, const in_scope_map& map) {
    auto cond    = resolve(e.condition, map);
    auto true_v  = resolve(e.value_true, map);
    auto false_v = resolve(e.value_false, map);
    auto true_t  = type_of(true_v);
    auto false_t = type_of(false_v);

    if (*true_t != *false_t) {
        throw std::runtime_error(fmt::format("type mismatch {} and {} between the true and false branches "
                                             "of the conditional statement at {}.",
                                             to_string(true_t), to_string(false_t), to_string(e.loc)));
    }

    return make_rexpr<resolved_conditional>(cond, true_v, false_v, true_t, e.loc);
}

r_expr resolve(const parsed_float& e, const in_scope_map& map) {
    using namespace parsed_unit_ir;
    auto f_val = e.value;
    auto f_type = to_type(e.unit);
    auto r_type = resolve_type(f_type, map.type_map);
    return make_rexpr<resolved_float>(f_val, r_type, e.loc);
}

r_expr resolve(const parsed_int& e, const in_scope_map& map) {
    using namespace parsed_unit_ir;
    auto i_val = e.value;
    auto i_type = to_type(e.unit);
    auto r_type = resolve_type(i_type, map.type_map);
    return make_rexpr<resolved_int>(i_val, r_type, e.loc);
}

r_expr resolve(const parsed_unary& e, const in_scope_map& map) {
    auto val = resolve(e.value, map);
    auto type = type_of(val);
    switch (e.op) {
        case unary_op::exp:
        case unary_op::log:
        case unary_op::cos:
        case unary_op::sin:
        case unary_op::exprelr: {
            auto q_type = std::get_if<resolved_quantity>(type.get());
            if (!q_type) {
                throw std::runtime_error(fmt::format("Cannot apply op {} to non-real type, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            if (!q_type->type.is_real()) {
                throw std::runtime_error(fmt::format("Cannot apply op {} to non-real type, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            break;
        }
        case unary_op::lnot: {
            if (!std::get_if<resolved_boolean>(type.get())) {
                throw std::runtime_error(fmt::format("Cannot apply op {} to non-boolean type, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            break;
        }
        case unary_op::neg: {
            if (std::get_if<resolved_record>(type.get())) {
                throw std::runtime_error(fmt::format("Cannot apply op {} to record type, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            break;
        }
        default: break;
    }
    return make_rexpr<resolved_unary>(e.op, val, type, e.loc);
}

r_expr resolve(const parsed_binary& e, const in_scope_map& map) {
    auto lhs_v = resolve(e.lhs, map);
    auto lhs_t = type_of(lhs_v);
    auto lhs_loc = location_of(lhs_v);

    if (e.op == binary_op::dot) {
        auto make_or_throw = [&](const resolved_record& r) {
            // Add fields to the local map
            auto available_map = map;
            for (auto [f_id, f_type]: r.fields) {
                available_map.local_map.insert_or_assign(f_id, resolved_argument(f_id, f_type, r.loc));
            }
            auto rhs_v = resolve(e.rhs, available_map);

            // Resolved rhs is expected to be one of the just added fields
            // But we need to check

            // Check that it is a resolved_argument
            auto* r_rhs = std::get_if<resolved_argument>(rhs_v.get());
            if (!r_rhs) {
                throw std::runtime_error(fmt::format("incompatible argument type to dot operator, at {}", to_string(e.loc)));
            }
            auto rhs_id = r_rhs->name;

            // Check that it is an argument of the record
            for (auto [f_id, f_type]: r.fields) {
                if (rhs_id == f_id) {
                    return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, f_type, e.loc);
                }
            }
            throw std::runtime_error(fmt::format("argument {} doesn't match any of the record fields, at {}",
                                                 to_string(rhs_v), to_string(lhs_loc)));
        };

        if (auto* lhs_rec = std::get_if<resolved_record>(lhs_t.get())) {
            return make_or_throw(*lhs_rec);
        }
    }

    // If it's not a field access, we can use the input map to resolve rhs
    auto rhs_v = resolve(e.rhs, map);
    auto rhs_t = type_of(rhs_v);
    auto rhs_loc = location_of(rhs_v);

    // In this case, neither lhs nor rhs can have record types.
    if (std::get_if<resolved_record>(rhs_t.get())) {
        throw std::runtime_error(fmt::format("Cannot apply op {} to record type, at {}",
                                             to_string(e.op), to_string(rhs_loc)));
    }
    if (std::get_if<resolved_record>(lhs_t.get())) {
        throw std::runtime_error(fmt::format("Cannot apply op {} to record type, at {}",
                                             to_string(e.op), to_string(lhs_loc)));
    }

    // And, they both have to have the same type.
    if ((std::get_if<resolved_boolean>(lhs_t.get()) && !std::get_if<resolved_boolean>(rhs_t.get())) ||
        (!std::get_if<resolved_boolean>(lhs_t.get()) && std::get_if<resolved_boolean>(rhs_t.get()))) {
        throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                             to_string(e.op), to_string(e.loc)));
    }

    auto lhs_q = std::get_if<resolved_quantity>(lhs_t.get());
    auto rhs_q = std::get_if<resolved_quantity>(rhs_t.get());
    switch (e.op) {
        case binary_op::min:
        case binary_op::max:
        case binary_op::add:
        case binary_op::sub: {
            // Only applicable if neither lhs nor rhs is a boolean type
            if (lhs_q && rhs_q) {
                if (lhs_q->type != rhs_q->type) {
                    throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                         to_string(e.op), to_string(e.loc)));
                }
                return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, lhs_t, e.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                   to_string(e.op), to_string(e.loc)));
        }
        case binary_op::mul: {
            // Only applicable if neither lhs nor rhs is a boolean type
            if (lhs_q && rhs_q) {
                auto comb_type = make_rtype<resolved_quantity>(lhs_q->type*rhs_q->type, e.loc);
                return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, comb_type, e.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                 to_string(e.op), to_string(e.loc)));
        }
        case binary_op::div: {
            // Only applicable if neither lhs nor rhs is a boolean type
            if (lhs_q && rhs_q) {
                auto comb_type = make_rtype<resolved_quantity>(lhs_q->type/rhs_q->type, e.loc);
                return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, comb_type, e.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                 to_string(e.op), to_string(e.loc)));
        }
        case binary_op::pow: {
            // Only applicable if neither lhs nor rhs is a boolean type and rhs is a real int type
            auto rhs_int = std::get_if<resolved_int>(rhs_v.get());
            if (!rhs_int) {
                throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            auto rhs_int_q = std::get_if<resolved_quantity>(rhs_int->type.get());
            if (!rhs_int_q || !rhs_int_q->type.is_real()) {
                throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            if (lhs_q && rhs_q) {
                auto comb_type = make_rtype<resolved_quantity>(lhs_q->type^rhs_int->value, e.loc);
                return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, comb_type, e.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                 to_string(e.op), to_string(e.loc)));
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
                                                         to_string(e.op), to_string(e.loc)));
                }
            }
            return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, make_rtype<resolved_boolean>(e.loc), e.loc);
        }
        case binary_op::land:
        case binary_op::lor: {
            return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, make_rtype<resolved_boolean>(e.loc), e.loc);
        }
        default: break;
    }
    return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, lhs_t, e.loc);
}

r_expr resolve(const parsed_identifier& e, const in_scope_map& map) {
    if (map.local_map.count(e.name)) {
        return make_rexpr<resolved_argument>(map.local_map.at(e.name));
    }
    if (map.param_map.count(e.name)) {
        return make_rexpr<resolved_parameter>(map.param_map.at(e.name));
    }
    if (map.const_map.count(e.name)) {
        return make_rexpr<resolved_constant>(map.const_map.at(e.name));
    }
    if (map.bind_map.count(e.name)) {
        return make_rexpr<resolved_bind>(map.bind_map.at(e.name));
    }
    if (map.state_map.count(e.name)) {
        return make_rexpr<resolved_state>(map.state_map.at(e.name));
    }
    throw std::runtime_error(fmt::format("undefined identifier {}, at {}",
                                         e.name, to_string(e.loc)));
}

resolved_mechanism resolve(const parsed_mechanism& e, const in_scope_map& map) {
    resolved_mechanism mech;
    auto available_map = map;
    for (const auto& c: e.constants) {
        auto val = resolve(c, available_map);
        mech.constants.push_back(val);
        auto const_val = std::get_if<resolved_constant>(val.get());
        if (!const_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected constant expression at {}",
                                                 to_string(location_of(val))));
        }
        available_map.const_map.insert({const_val->name, *const_val});
    }
    for (const auto& c: e.parameters) {
        auto val =resolve(c, available_map);
        mech.parameters.push_back(val);
        auto param_val = std::get_if<resolved_parameter>(val.get());
        if (!param_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected parameter expression at {}",
                                                 to_string(location_of(val))));
        }
        available_map.param_map.insert({param_val->name, *param_val});
    }
    for (const auto& c: e.bindings) {
        auto val = resolve(c, available_map);
        mech.bindings.push_back(val);
        auto bind_val = std::get_if<resolved_bind>(val.get());
        if (!bind_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected bind expression at {}",
                                                 to_string(location_of(val))));
        }
        available_map.bind_map.insert({bind_val->name, *bind_val});
    }
    // For records, create 2 records: regular and prime
    for (const auto& r: e.records) {
        auto rec = std::get_if<parsed_record_alias>(r.get());
        if (!rec) {
            throw std::runtime_error(fmt::format("internal compiler error, expected record expression at {}",
                                                 to_string(location_of(r))));
        }

        // regular
        auto resolved_record = resolve(r, available_map);
        auto resolved_record_val = std::get_if<resolved_record_alias>(resolved_record.get());
        if (!resolved_record_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected record expression at {}",
                                                 to_string(location_of(r))));
        }
        available_map.type_map.insert({resolved_record_val->name, resolved_record_val->type});

        // prime
        auto derived_parsed_record_type = derive(resolved_record_val->type);
        if (derived_parsed_record_type) {
            available_map.type_map.insert({resolved_record_val->name+"'", derived_parsed_record_type.value()});
        }
    }
    for (const auto& c: e.states) {
        auto val = resolve(c, available_map);
        mech.states.push_back(val);
        auto state_val = std::get_if<resolved_state>(val.get());
        if (!state_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected state expression at {}",
                                                 to_string(location_of(val))));
        }
        available_map.state_map.insert({state_val->name, *state_val});
    }
    for (const auto& c: e.functions) {
        auto val = resolve(c, available_map);
        mech.functions.push_back(val);
        auto func_val = std::get_if<resolved_function>(val.get());
        if (!func_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected funcyion expression at {}",
                                                 to_string(location_of(val))));
        }
        available_map.func_map.insert({func_val->name, *func_val});
    }
    for (const auto& c: e.initializations) {
        mech.initializations.push_back(resolve(c, available_map));
    }
    for (const auto& c: e.evolutions) {
        mech.evolutions.push_back(resolve(c, available_map));
    }
    for (const auto& c: e.effects) {
        mech.effects.push_back(resolve(c, available_map));
    }
    for (const auto& c: e.exports) {
        mech.exports.push_back(resolve(c, available_map));
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return mech;
}

r_expr resolve(const parsed_ir::p_expr& e, const in_scope_map& map) {
    return std::visit([&](auto&& c){return resolve(c, map);}, *e);
}

// resolved_mechanism
std::string to_string(const resolved_mechanism& e, bool include_type, int indent) {
    auto indent_str = std::string(indent*2, ' ');
    std::string str = indent_str + "(module_expr " + e.name + " " + to_string(e.kind) + "\n";
    for (const auto& p: e.parameters) {
        str += to_string(p, include_type, indent+1) + "\n";
    }
    for (const auto& p: e.constants) {
        str += to_string(p, include_type, indent+1) + "\n";
    }
    for (const auto& p: e.states) {
        str += to_string(p, include_type, indent+1) + "\n";
    }
    for (const auto& p: e.bindings) {
        str += to_string(p, include_type, indent+1) + "\n";
    }
    for (const auto& p: e.functions) {
        str += to_string(p, include_type, indent+1) + "\n";
    }
    for (const auto& p: e.initializations) {
        str += to_string(p, include_type, indent+1) + "\n";
    }
    for (const auto& p: e.evolutions) {
        str += to_string(p, include_type, indent+1) + "\n";
    }
    for (const auto& p: e.effects) {
        str += to_string(p, include_type, indent+1) + "\n";
    }
    for (const auto& p: e.exports) {
        str += to_string(p, include_type, indent+1) + "\n";
    }
    return str + to_string(e.loc) + ")";
}

// resolved_parameter
std::string to_string(const resolved_parameter& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_parameter\n";
    str += double_indent + e.name + "\n";
    str += to_string(e.value, false, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_constant
std::string to_string(const resolved_constant& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_constant\n";
    str += double_indent + e.name + "\n";
    str += to_string(e.value, false, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1) + "\n";
    return str + ")";
}

// resolved_state
std::string to_string(const resolved_state& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_state\n";
    str += double_indent + e.name;
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_record_alias
std::string to_string(const resolved_record_alias& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_record_alias\n";
    str += double_indent + e.name;
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_function
std::string to_string(const resolved_function& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_function\n";
    str += double_indent + e.name +  "\n";
    str += to_string(e.type, indent+1) + "\n";

    str += double_indent + "(\n";
    for (const auto& f: e.args) {
        str += to_string(f, true, indent+2) + "\n";
    }
    str += double_indent + ")";
    if (include_type) str += "\n" + to_string(e.body, false, indent+1);
    return str + ")";
}

// resolved_bind
std::string to_string(const resolved_bind& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_bind\n";
    str += (double_indent + to_string(e.bind));
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    str += double_indent + e.name;
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_initial
std::string to_string(const resolved_initial& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_initial\n";
    str += to_string(e.identifier, false, indent+1) + "\n";
    str += to_string(e.value, false, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_evolve
std::string to_string(const resolved_evolve& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_evolve\n";
    str += to_string(e.identifier, false, indent+1) + "\n";
    str += to_string(e.value, false, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_effect
std::string to_string(const resolved_effect& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_effect\n";
    str += (double_indent + to_string(e.effect));
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    str += to_string(e.value, false, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_export
std::string to_string(const resolved_export& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_export\n";
    str += to_string(e.identifier, false, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_call
std::string to_string(const resolved_call& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_call\n";
    str += double_indent + std::get<resolved_function>(*e.f_identifier).name;
    for (const auto& f: e.call_args) {
        str += "\n" + to_string(f, false, indent+1);
    }
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_object
std::string to_string(const resolved_object& e, bool include_type, int indent) {
    assert(e.record_fields.size() == e.record_values.size());

    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_object";
    if (e.r_identifier) str += "\n" + double_indent + std::get<resolved_record_alias>(*e.r_identifier.value()).name;
    for (unsigned i = 0; i < e.record_fields.size(); ++i) {
        str += "\n" + double_indent + "(\n";
        str += to_string(e.record_fields[i], false, indent+2) + "\n";
        str += to_string(e.record_values[i], false, indent+2) + "\n";
        str += double_indent + ")";
    }
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_let
std::string to_string(const resolved_let& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_let\n";
    str += to_string(e.identifier, false, indent+1) + "\n";
    str += to_string(e.value, true, indent+1) + "\n";
    str += to_string(e.body, true, indent+1);
    auto type = to_string(e.type, indent+1);
    if (include_type) str += "\n" + type;
    return str + ")";
}

// resolved_conditional
std::string to_string(const resolved_conditional& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_conditional\n";
    str += to_string(e.condition, false, indent+1) + "\n";
    str += to_string(e.value_true, false, indent+1) + "\n";
    str += to_string(e.value_false, false, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_float
std::string to_string(const resolved_float& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_float\n";
    str += double_indent + std::to_string(e.value);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_int
std::string to_string(const resolved_int& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_int\n";
    str += double_indent + std::to_string(e.value);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_unary
std::string to_string(const resolved_unary& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_unary " + to_string(e.op) + "\n";
    str += to_string(e.arg, false, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_binary
std::string to_string(const resolved_binary& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_binary " + to_string(e.op) + "\n";
    str += to_string(e.lhs, false, indent+1) + "\n";
    str += to_string(e.rhs, false, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

std::string to_string(const resolved_argument& e, bool include_type, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_argument \n";
    str += double_indent + e.name;
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

std::string to_string(const r_expr & e, bool include_type, int indent) {
    return std::visit([&](auto&& c){return to_string(c, include_type, indent);}, *e);
}

// equality comparison
bool operator==(const resolved_parameter& lhs, const resolved_parameter& rhs) {
    return (lhs.name == rhs.name) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_constant& lhs, const resolved_constant& rhs) {
    return (lhs.name == rhs.name) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_state& lhs, const resolved_state& rhs) {
    return (lhs.name == rhs.name) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_record_alias& lhs, const resolved_record_alias& rhs) {
    return (lhs.name == rhs.name) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_function& lhs, const resolved_function& rhs) {
    if (lhs.args.size() != rhs.args.size()) return false;
    for (unsigned i = 0; i < lhs.args.size(); ++i) {
        if (*lhs.args[i] != *rhs.args[i]) return false;
    }
    return (lhs.name == rhs.name) && (*lhs.body == *rhs.body) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_argument& lhs, const resolved_argument& rhs) {
    return (lhs.name == rhs.name) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_bind& lhs, const resolved_bind& rhs) {
    return (lhs.bind == rhs.bind) && (lhs.ion == rhs.ion) && (lhs.name == rhs.name) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_initial& lhs, const resolved_initial& rhs) {
    return (*lhs.identifier == *rhs.identifier) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_evolve& lhs, const resolved_evolve& rhs) {
    return (*lhs.identifier == *rhs.identifier) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_effect& lhs, const resolved_effect& rhs) {
    return (lhs.effect == rhs.effect) && (lhs.ion == rhs.ion) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_export& lhs, const resolved_export& rhs) {
    return (*lhs.identifier == *rhs.identifier) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_call& lhs, const resolved_call& rhs) {
    if (lhs.call_args.size() != rhs.call_args.size()) return false;
    for (unsigned i = 0; i < lhs.call_args.size(); ++i) {
        if (*lhs.call_args[i] != *rhs.call_args[i]) return false;
    }
    return  (*lhs.f_identifier == *rhs.f_identifier) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_object& lhs, const resolved_object& rhs) {
    if (lhs.record_fields.size() != rhs.record_fields.size()) return false;
    if (lhs.record_values.size() != rhs.record_values.size()) return false;
    for (unsigned i = 0; i < lhs.record_fields.size(); ++i) {
        if (*lhs.record_fields[i] != *rhs.record_fields[i]) return false;
    }
    for (unsigned i = 0; i < lhs.record_values.size(); ++i) {
        if (*lhs.record_values[i] != *rhs.record_values[i]) return false;
    }
    return  (*lhs.r_identifier == *rhs.r_identifier) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_let& lhs, const resolved_let& rhs) {
    return (*lhs.identifier == *rhs.identifier) && (*lhs.value == *rhs.value) &&
           (*lhs.body == *rhs.body) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_conditional& lhs, const resolved_conditional& rhs) {
    return (*lhs.condition == *rhs.condition) && (*lhs.value_true == *rhs.value_true) &&
           (*lhs.value_false == *rhs.value_false) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_float& lhs, const resolved_float& rhs) {
    return (lhs.value == rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_int& lhs, const resolved_int& rhs) {
    return (lhs.value == rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_unary& lhs, const resolved_unary& rhs) {
    return (lhs.op == rhs.op) && (*lhs.arg == *rhs.arg) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_binary& lhs, const resolved_binary& rhs) {
    return (lhs.op == rhs.op) && (*lhs.lhs == *rhs.lhs) && (*lhs.rhs == *rhs.rhs) && (*lhs.type == *rhs.type);
}

bool operator!=(const resolved_parameter& lhs, const resolved_parameter& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_constant& lhs, const resolved_constant& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_state& lhs, const resolved_state& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_record_alias& lhs, const resolved_record_alias& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_function& lhs, const resolved_function& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_argument& lhs, const resolved_argument& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_bind& lhs, const resolved_bind& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_initial& lhs, const resolved_initial& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_evolve& lhs, const resolved_evolve& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_effect& lhs, const resolved_effect& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_export& lhs, const resolved_export& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_call& lhs, const resolved_call& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_object& lhs, const resolved_object& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_let& lhs, const resolved_let& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_conditional& lhs, const resolved_conditional& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_float& lhs, const resolved_float& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_int& lhs, const resolved_int& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_unary& lhs, const resolved_unary& rhs) {return !(lhs == rhs);}
bool operator!=(const resolved_binary& lhs, const resolved_binary& rhs) {return !(lhs == rhs);}


// common member getters
r_type type_of(const r_expr& e) {
    return std::visit([](auto&& c){return c.type;}, *e);
}

src_location location_of(const r_expr& e) {
    return std::visit([](auto&& c){return c.loc;}, *e);
}

} // namespace al
} // namespace parsed_ir
