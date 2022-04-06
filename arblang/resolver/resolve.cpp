#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

#include <fmt/core.h>

#include <arblang/parser/parsed_expressions.hpp>
#include <arblang/resolver/resolved_types.hpp>
#include <arblang/resolver/resolve.hpp>
#include <arblang/util/common.hpp>

namespace al {
namespace resolved_ir {
using namespace parsed_ir;
using namespace resolved_type_ir;

// Resolver
// Performs type checking and name resolution and produces `resolved` expressions,
// the main IR of the compiler.
// When type checking is performed, units can be dropped (normalization is expected
// to have already been performed). Each expression has a type, and quantity types
// are represented in terms of the 6 base SI units (excluding luminosity):
// length, time, amount of substance, electric current, temperature, mass.
// When name resolution is performed, a `resolved_variable` will contain a link
// to its definition. e.g.
//   let a = x + 5;
//   let b = a/3;
// It would be possible to create `b = (x+5)/3` from the second expression because
// after name resolution, `a` contains a "link" to its value.

void check_duplicate(const std::string& name, const src_location& loc, const in_scope_map& map) {
    if (map.param_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate definition, found at {} and {}",
                                             to_string(location_of(map.param_map.at(name))), to_string(loc)));
    }
    if (map.const_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate constant name, also found at {}",
                                             to_string(location_of(map.const_map.at(name))), to_string(loc)));
    }
    if (map.bind_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate binding name, also found at {}",
                                             to_string(location_of(map.bind_map.at(name))), to_string(loc)));
    }
    if (map.state_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate state name, also found at {}",
                                             to_string(location_of(map.state_map.at(name))), to_string(loc)));
    }
}

r_expr resolve(const parsed_parameter& e, const in_scope_map& map) {
    auto id = is_parsed_identifier(e.identifier);
    if (!id) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                             to_string(e.identifier), to_string(location_of(e.identifier))));
    }

    auto p_name = id->name;
    check_duplicate(p_name, id->loc, map);

    // Not every variable in scope is available for use
    // by a parameter expression, only constant expressions,
    // other parameter expressions, and local expressions
    // (let expressions) introduced within the parameter
    // expression.
    auto available_map = map;
    available_map.bind_map.clear();
    available_map.state_map.clear();

    auto p_val = resolve(e.value, available_map);;
    auto p_type = type_of(p_val);

    // Check that the type of the identifier, if it exists,
    // matches the type of the value.
    if (id->type) {
        auto id_type = resolve_type(id->type.value(), map.type_map);
        if (*id_type != *p_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(p_type), to_string(id->loc)));
        }
    }
    return make_rexpr<resolved_parameter>(p_name, p_val, p_type, e.loc);
}

r_expr resolve(const parsed_constant& e, const in_scope_map& map) {
    auto id = is_parsed_identifier(e.identifier);
    if (!id) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                             to_string(e.identifier), to_string(location_of(e.identifier))));
    }

    auto c_name = id->name;
    check_duplicate(c_name, id->loc, map);

    // Not every variable in scope is available for use
    // by a constant expression, only other constant
    // expressions, and local expressions (let expressions)
    // introduced within the constant expression.
    auto available_map = map;
    available_map.param_map.clear();
    available_map.bind_map.clear();
    available_map.state_map.clear();

    auto c_val  = resolve(e.value, available_map);
    auto c_type = type_of(c_val);

    // Check that the type of the identifier, if it exists,
    // matches the type of the value.
    if (id->type) {
        auto id_type = resolve_type(id->type.value(), map.type_map);
        if (*id_type != *c_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(c_type), to_string(id->loc)));
        }
    }

    return make_rexpr<resolved_constant>(c_name, c_val, c_type, e.loc);
}

r_expr resolve(const parsed_state& e, const in_scope_map& map) {
    auto id = is_parsed_identifier(e.identifier);
    if (!id) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                             to_string(e.identifier), to_string(location_of(e.identifier))));
    }

    auto s_name = id->name;
    check_duplicate(s_name, id->loc, map);

    auto s_type = resolve_type(id->type.value(), map.type_map);

    // Check that the state identifier has a type, because it needs a type.
    if (!id->type) {
        throw std::runtime_error(fmt::format("state identifier {} at {} missing quantity type.",
                                             s_name, to_string(id->loc)));
    }
    return make_rexpr<resolved_state>(s_name, s_type, e.loc);
}

r_expr resolve(const parsed_bind& e, const in_scope_map& map) {
    auto id = is_parsed_identifier(e.identifier);
    if (!id) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                             to_string(e.identifier), to_string(location_of(e.identifier))));
    }

    auto b_name = id->name;
    check_duplicate(b_name, id->loc, map);

    auto b_type = resolve_type(e.bind, e.loc);

    // Check that the type of the identifier, if it exists,
    // matches the type of the bindable.
    if (id->type) {
        auto id_type = resolve_type(id->type.value(), map.type_map);
        if (*id_type != *b_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(b_type), to_string(id->loc)));
        }
    }
    return make_rexpr<resolved_bind>(b_name, e.bind, e.ion, b_type, e.loc);
}

r_expr resolve(const parsed_record_alias& e, const in_scope_map& map) {
    if (map.type_map.count(e.name)) {
        throw std::runtime_error(fmt::format("duplicate record alias name, also found at {}",
                                             to_string(location_of(map.type_map.at(e.name)))));
    }
    auto a_type = resolve_type(e.type, map.type_map);
    return make_rexpr<resolved_record_alias>(e.name, a_type, e.loc);
}

r_expr resolve(const parsed_function& e, const in_scope_map& map) {
    auto f_name = e.name;
    if (map.func_map.count(f_name)) {
        throw std::runtime_error(fmt::format("duplicate function name, also found at {}",
                                             to_string(location_of(map.param_map.at(f_name)))));
    }

    // Add the function arguments to the scope
    auto available_map = map;
    std::vector<r_expr> f_args;
    for (const auto& a: e.args) {
        auto arg_id = is_parsed_identifier(a);
        if (!arg_id) {
            throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                                 to_string(a), to_string(location_of(a))));
        }
        auto a_id = arg_id.value();

        // Check that function arguments have defined types.
        if (!a_id.type) {
            throw std::runtime_error(fmt::format("function argument {} at {} missing quantity type.",
                                                 a_id.name, to_string(a_id.loc)));
        }
        auto a_type = resolve_type(a_id.type.value(), map.type_map);

        // Function arguments are resolved_arguments (argument with no
        // pointer unknown value). They are replaced with resolved_variables
        // during inlining.
        auto f_a = make_rexpr<resolved_argument>(a_id.name, a_type, a_id.loc);

        f_args.push_back(f_a);
        available_map.local_map.insert_or_assign(a_id.name, f_a);
    }

    // Resolve the function body now that the arguments are in scope
    auto f_body = resolve(e.body, available_map);
    auto f_type = type_of(f_body);

    // Check that the type of the body expression, if it exists,
    // matches the type of the return value of the function.
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
    auto id = is_parsed_identifier(e.identifier);
    if (!id) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                             to_string(e.identifier), to_string(location_of(e.identifier))));
    }
    auto i_name = id->name;

    // Initial expressions can only write to state variables
    if (!map.state_map.count(i_name)) {
        throw std::runtime_error(fmt::format("variable {} initialized at {} is not a state variable.",
                                             i_name, to_string(e.loc)));
    }

    // Resolve the value of the initial expression
    auto i_val = resolve(e.value, map);
    auto i_type = type_of(i_val);

    // Check that the type of the identifier, if it exists,
    // matches the type of the value
    if (id->type) {
        auto id_type = resolve_type(id->type.value(), map.type_map);
        if (*id_type != *i_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(i_type), to_string(id->loc)));
        }
    }

    // Note: the initial expression refers to the state variable
    // being updated using a resolved_argument, so s_expr is expected
    // to be a resolved_argument.
    auto s_expr = map.state_map.at(i_name);

    // Check that the type of the state variable matches the
    // type of the value.
    if (*type_of(s_expr) != *i_type) {
        throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                             to_string(type_of(s_expr)), to_string(i_type), to_string(id->loc)));
    }

    return make_rexpr<resolved_initial>(s_expr, i_val, i_type, e.loc);
}

r_expr resolve(const parsed_on_event& e, const in_scope_map& map) {
    auto id = is_parsed_identifier(e.identifier);
    if (!id) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                             to_string(e.identifier), to_string(location_of(e.identifier))));
    }
    auto i_name = id->name;

    // on_event expressions can only write to state variables
    if (!map.state_map.count(i_name)) {
        throw std::runtime_error(fmt::format("variable {} modified in on_event at {} is not a state variable.",
                                             i_name, to_string(e.loc)));
    }

    // on_event expressions contain an optional argument,
    // sent from the simulator (weight argument in a connection).
    // This argument needs to have a user-defined type.
    auto a_id = is_parsed_identifier(e.argument);
    if (!a_id) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                             to_string(e.argument), to_string(location_of(e.argument))));
    }
    if (!a_id->type) {
        throw std::runtime_error(fmt::format("on_event argument {} at {} missing quantity type.",
                                             a_id->name, to_string(a_id->loc)));
    }

    // The argument can only be a scalar value,
    // therefore it can not have a record type
    auto a_type = resolve_type(a_id->type.value(), map.type_map);
    if (is_resolved_record_type(a_type)) {
        throw std::runtime_error(fmt::format("on_event argument {} at {} has invalid quantity type {}; "
                                             "a single (non-record) argument is expected.",
                                             a_id->name, to_string(a_id->loc), to_string(a_type)));
    }

    // The argument can be used in the value of the on_event
    // expression. It needs to be part of the scope. It is added
    // to the scope as a resolved_argument.
    auto available_map = map;
    auto o_arg = make_rexpr<resolved_argument>(a_id->name, a_type, a_id->loc);
    available_map.local_map.insert_or_assign(a_id->name, o_arg);

    // Resolve the value of the 'on_event' expression
    auto o_val = resolve(e.value, available_map);
    auto o_type = type_of(o_val);

    // Check that the type of the identifier matches the type of the value
    if (id->type) {
        auto id_type = resolve_type(id->type.value(), map.type_map);
        if (*id_type != *o_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(o_type), to_string(id->loc)));
        }
    }
    // Note: the on_event expression refers to the state variable
    // being updated using a resolved_argument, so s_expr is expected
    // to be a resolved_argument.
    auto s_expr = map.state_map.at(i_name);
    return make_rexpr<resolved_on_event>(o_arg, s_expr, o_val, o_type, e.loc);
}

r_expr resolve(const parsed_evolve& e, const in_scope_map& map) {
    auto id = is_parsed_identifier(e.identifier);
    if (!id) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                             to_string(e.identifier), to_string(location_of(e.identifier))));
    }

    // Evolve expressions can only write state variable derivatives.
    // State variable a will have a derivative a'
    auto e_name = id->name;
    if (e_name.back() != '\'') {
        throw std::runtime_error(fmt::format("variable {} evolved at {} is not a derivative.", e_name, to_string(e.loc)));
    }
    e_name.pop_back();
    if (!map.state_map.count(e_name)) {
        throw std::runtime_error(fmt::format("variable {} evolved at {} is not a state variable.", e_name, to_string(e.loc)));
    }

    // Resolve the value of the evolve expression
    auto e_val = resolve(e.value, map);
    auto e_type = type_of(e_val);

    // Check that the type of the identifier matches the type of the value
    if (id->type) {
        auto id_type = resolve_type(id->type.value(), map.type_map);
        if (*id_type != *e_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(e_type), to_string(id->loc)));
        }
    }

    // Note: the evolve expression refers to the state variable
    // being updated using a resolved_argument, so s_expr is expected
    // to be a resolved_argument.
    auto s_expr = map.state_map.at(e_name);

    // Check that the type of the value matches the type of the derivative of the state
    auto s_type = derive(type_of(s_expr)).value();
    if (*s_type != *e_type) {
        throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                             to_string(s_type), to_string(e_type), to_string(id->loc)));
    }
    return make_rexpr<resolved_evolve>(s_expr, e_val, e_type, e.loc);
}

r_expr resolve(const parsed_effect& e, const in_scope_map& map) {
    auto e_effect = e.effect;
    auto e_ion = e.ion;

    // Resolve the value of the 'effect' expression
    auto e_val = resolve(e.value, map);
    auto e_type = type_of(e_val);
    auto f_type = resolve_type(e_effect, e.loc);

    // Check that the type of the affected quantity matches the type of the value
    if (*f_type != *e_type) {
        throw std::runtime_error(fmt::format("type mismatch between {} and {} in effect expression at {}.",
                                             to_string(f_type), to_string(e_type), to_string(e.loc)));
    }

    return make_rexpr<resolved_effect>(e_effect, e_ion, e_val, e_type, e.loc);
}

r_expr resolve(const parsed_export& e, const in_scope_map& map) {
    auto id = is_parsed_identifier(e.identifier);
    if (!id) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                             to_string(e.identifier), to_string(location_of(e.identifier))));
    }

    auto p_name = id->name;

    // Check that the identifier being exported is a parameter.
    if (!map.param_map.count(p_name)) {
        throw std::runtime_error(fmt::format("variable {} exported at {} is not a parameter.",
                                             p_name, to_string(e.loc)));
    }

    // Note: the export expression refers to the parameter variable
    // being exported using a resolved_argument, so p_expr is expected
    // to be a resolved_argument.
    auto p_expr  = map.param_map.at(p_name);
    return make_rexpr<resolved_export>(p_expr, type_of(p_expr), e.loc);
}

r_expr resolve(const parsed_call& e, const in_scope_map& map) {
    auto f_name = e.function_name;
    if (!map.func_map.count(f_name)) {
        throw std::runtime_error(fmt::format("function {} called at {} is not defined.", f_name, to_string(e.loc)));
    }
    auto f_expr = map.func_map.at(f_name);
    auto func = is_resolved_function(f_expr).value();

    // Resolve the call arguments
    std::vector<r_expr> c_args;
    for (const auto& a: e.call_args) {
        c_args.push_back(resolve(a, map));
    }

    // Check that the types of the call arguments match
    // the types of the called function arguments.
    if (func.args.size() != c_args.size()) {
        throw std::runtime_error(fmt::format("argument count mismatch when calling function {} at {}.",
                                             f_name, to_string(e.loc)));
    }
    for (unsigned i=0; i < func.args.size(); ++i) {
        auto f_arg_type = type_of(func.args[i]);
        auto c_arg_type = type_of(c_args[i]);
        if (*f_arg_type != *c_arg_type) {
            throw std::runtime_error(
                    fmt::format("type mismatch between {} and {} of argument {} of function call {} at {}.",
                                to_string(f_arg_type), to_string(c_arg_type), std::to_string(i), f_name, to_string(e.loc)));
        }
    }

    // The 'call' expression refers to the called function using only a string.
    // We don't need a pointer to the actual function definition.
    // At the time of inlining, we provide a map from func_name -> definition,
    // and the call expression disappears.
    return make_rexpr<resolved_call>(f_name, c_args, func.type, e.loc);
}

r_expr resolve(const parsed_object& e, const in_scope_map& map) {
    assert(e.record_fields.size() == e.record_values.size());

    // Resolve the object field values
    std::vector<r_expr> f_values;
    std::vector<r_type> f_types;
    for (const auto& v: e.record_values) {
        f_values.push_back(resolve(v, map));
        f_types.push_back(type_of(f_values.back()));
    }

    // Generate the resolved object
    std::vector<r_expr> o_fields;
    std::vector<std::pair<std::string, r_type>> o_types;
    for (unsigned i = 0; i < e.record_fields.size(); ++i) {
        auto f_id = is_parsed_identifier(e.record_fields[i]);
        if (!f_id) {
            throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                                 to_string(e.record_fields[i]), to_string(location_of(e.record_fields[i]))));
        }

        // Check that the field identifier type, if it exists,
        // matches the field value type.
        if (f_id->type) {
            auto fid_type = resolve_type(f_id->type.value(), map.type_map);
            if (*fid_type != *f_types[i]) {
                throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                     to_string(fid_type), to_string(f_types[i]), to_string(f_id->loc)));
            }
        }

        // The field variable is a resolved_variable with a pointer
        // to the actual value of the identifier.
        o_fields.push_back(make_rexpr<resolved_variable>(f_id->name, f_values[i], f_types[i], f_id->loc));

        // The individual types of the fields are needed
        // to generate the type of the object
        o_types.emplace_back(f_id->name, f_types[i]);
    }

    // Generate the type of the object
    auto o_type = make_rtype<resolved_record>(o_types, e.loc);

    // Check that the type of the object matches the type of the
    // record alias used to construct the object, if it is provided.
    if (e.record_name) {
        auto r_name = e.record_name.value();
        if (!map.type_map.count(r_name)) {
            throw std::runtime_error(
                    fmt::format("record {} referenced at {} is not defined.", r_name, to_string(e.loc)));
        }
        auto r_type = map.type_map.at(r_name);
        if (*r_type != *o_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} while constructing object {} at {}.",
                                                 to_string(r_type), to_string(o_type), r_name, to_string(e.loc)));
        }
    }

    return make_rexpr<resolved_object>(o_fields, o_type, e.loc);
}

r_expr resolve(const parsed_let& e, const in_scope_map& map) {
    auto id = is_parsed_identifier(e.identifier);
    if (!id) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected identifier instead of {} at {}.",
                                             to_string(e.identifier), to_string(location_of(e.identifier))));
    }
    check_duplicate(id->name, id->loc, map);

    // Resolve the value of the let expression
    auto v_expr = resolve(e.value, map);
    auto v_type = type_of(v_expr);

    // Check that the type of the let bound identifier,
    // if it exists, matches the type of the value.
    if (id->type) {
        auto id_type = resolve_type(id->type.value(), map.type_map);
        if (*id_type != *v_type) {
            throw std::runtime_error(fmt::format("type mismatch between {} and {} at {}.",
                                                 to_string(id_type), to_string(v_type), to_string(id->loc)));
        }
    }

    // The let bound variable is a resolved_variable with a pointer to
    // the actual value of the variable.
    auto v_var = make_rexpr<resolved_variable>(id->name, v_expr, v_type, id->loc);

    // Add the let bound variable to the scope of the expression
    auto available_map = map;
    available_map.local_map.insert_or_assign(id->name, v_var);

    // Resolve the body of the let expression
    auto b_expr = resolve(e.body, available_map);
    auto b_type = type_of(b_expr);

    return make_rexpr<resolved_let>(v_var, b_expr, b_type, e.loc);
}

// The with expression becomes an equivalent let expression
r_expr resolve(const parsed_with& e, const in_scope_map& map) {
    // Resolve the value of the with expression to get the type
    auto v_expr = resolve(e.value, map);
    auto v_type = type_of(v_expr);

    // Transform with expression into nested let expressions
    std::vector<parsed_let> let_vars;
    if (auto v_rec = is_resolved_record_type(v_type)) {
        for (const auto& v_field: v_rec->fields) {
            auto iden = make_pexpr<parsed_identifier>(v_field.first, e.loc);
            auto value = make_pexpr<parsed_binary>(binary_op::dot, e.value, iden, e.loc);
            let_vars.push_back(parsed_let(iden, value, {}, e.loc));
        }
    }
    else {
        throw std::runtime_error(fmt::format("with value {} referenced at {} is not a record type.",
                                             to_string(e.value), to_string(e.loc)));
    }

    // Nest the let statements
    let_vars.back().body = e.body;
    for (int i = (int)let_vars.size()-2; i >=0; i--) {
        let_vars[i].body = make_pexpr<parsed_let>(let_vars[i+1]);
    }
    auto equivalent_let = make_pexpr<parsed_let>(let_vars.front());

    // Resolve the equivalent let_statement
    return resolve(equivalent_let, map);
}

r_expr resolve(const parsed_conditional& e, const in_scope_map& map) {
    auto cond    = resolve(e.condition, map);
    auto true_v  = resolve(e.value_true, map);
    auto false_v = resolve(e.value_false, map);
    auto true_t  = type_of(true_v);
    auto false_t = type_of(false_v);

    // Check that the type of the true and false branches
    // of the if_expression have the same type.
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
            auto q_type = is_resolved_quantity_type(type);
            if (!q_type || !q_type->type.is_real()) {
                throw std::runtime_error(fmt::format("Cannot apply op {} to non-real type, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            break;
        }
        case unary_op::lnot: {
            if (!is_resolved_bool_type(type)) {
                throw std::runtime_error(fmt::format("Cannot apply op {} to non-boolean type, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            break;
        }
        case unary_op::neg: {
            if (is_resolved_record_type(type)) {
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
    // Resolve the lhs of the expression
    auto lhs_v = resolve(e.lhs, map);
    auto lhs_t = type_of(lhs_v);
    auto lhs_loc = location_of(lhs_v);

    // If the binary operator is a dot, generate a `resolved_field_access`
    if (e.op == binary_op::dot) {
        if (auto lhs_rec = is_resolved_record_type(lhs_t)) {
            // rhs needs to be an identifier
            auto rhs = is_parsed_identifier(e.rhs);
            if (!rhs) {
                throw std::runtime_error(fmt::format("incompatible argument type to dot operator, at {}", to_string(e.loc)));
            }
            auto rhs_id = rhs->name;

            // Check that it is an argument of the record.
            for (auto [f_id, f_type]: lhs_rec->fields) {
                if (rhs_id == f_id) {
                    return make_rexpr<resolved_field_access>(lhs_v, rhs_id, f_type, e.loc);
                }
            }
            throw std::runtime_error(fmt::format("argument {} doesn't match any of the record fields, at {}",
                                                 rhs_id, to_string(lhs_loc)));
        } else {
            // If the lhs doesn't have a record type, throw an exception
            throw std::runtime_error(fmt::format("lhs of dot operator {} doesn't have a record type, at {}",
                                                 to_string(lhs_v), to_string(lhs_loc)));
        }
    }

    // If it's not a field access, we can use the input map to resolve rhs
    auto rhs_v = resolve(e.rhs, map);
    auto rhs_t = type_of(rhs_v);
    auto rhs_loc = location_of(rhs_v);

    // Neither lhs nor rhs can have record types.
    if (is_resolved_record_type(rhs_t)) {
        throw std::runtime_error(fmt::format("Cannot apply op {} to record type, at {}",
                                             to_string(e.op), to_string(rhs_loc)));
    }
    if (is_resolved_record_type(lhs_t)) {
        throw std::runtime_error(fmt::format("Cannot apply op {} to record type, at {}",
                                             to_string(e.op), to_string(lhs_loc)));
    }

    // And, they have to either both be boolean or neither.
    auto lhs_bool = is_resolved_bool_type(lhs_t);
    auto rhs_bool = is_resolved_bool_type(rhs_t);
    if ((lhs_bool && !rhs_bool) || (!lhs_bool && rhs_bool)) {
        throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                             to_string(e.op), to_string(e.loc)));
    }

    auto lhs_q = is_resolved_quantity_type(lhs_t);
    auto rhs_q = is_resolved_quantity_type(rhs_t);

    switch (e.op) {
        case binary_op::min:
        case binary_op::max:
        case binary_op::add:
        case binary_op::sub: {
            // Only applicable if neither lhs nor rhs is a boolean type
            if (lhs_q && rhs_q && (lhs_q->type == rhs_q->type)) {
                return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, lhs_t, e.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                 to_string(e.op), to_string(e.loc)));
        }
        case binary_op::mul: {
            // Only applicable if neither lhs nor rhs is a boolean type
            if (lhs_q && rhs_q) {
                return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, e.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                 to_string(e.op), to_string(e.loc)));
        }
        case binary_op::div: {
            // Only applicable if neither lhs nor rhs is a boolean type
            if (lhs_q && rhs_q) {
                return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, e.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                 to_string(e.op), to_string(e.loc)));
        }
        case binary_op::pow: {
            // Only applicable if rhs has a real type and lhs has a quantity type.
            if (!rhs_q || !rhs_q->type.is_real()) {
                throw std::runtime_error(fmt::format("incompatible rhs argument type to op {}, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            if (!lhs_q) {
                throw std::runtime_error(fmt::format("incompatible lhs argument type to op {}, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, e.loc);
        }
        case binary_op::lt:
        case binary_op::le:
        case binary_op::gt:
        case binary_op::ge:
        case binary_op::eq:
        case binary_op::ne: {
            // Only applicable if neither lhs nor rhs is a boolean type
            if (lhs_q && rhs_q && (lhs_q->type == rhs_q->type)) {
                return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, e.loc);
            }
            throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                 to_string(e.op), to_string(e.loc)));
        }
        case binary_op::land:
        case binary_op::lor: {
            if (lhs_q && rhs_q && (lhs_q->type != rhs_q->type)) {
                throw std::runtime_error(fmt::format("incompatible arguments types to op {}, at {}",
                                                     to_string(e.op), to_string(e.loc)));
            }
            return make_rexpr<resolved_binary>(e.op, lhs_v, rhs_v, e.loc);
        }
        default: throw std::runtime_error(fmt::format("Internal compiler error, unhandled operator {}",
                                                      to_string(e.op)));
    }
}

r_expr resolve(const parsed_identifier& e, const in_scope_map& map) {
    // parsed_identifiers are:
    // 1. resolved_argument if they are function arguments or constant/parameter/state/bind
    // 2. resolved_variable if they are object fields or bound let variables
    if (map.local_map.count(e.name)) {
        return map.local_map.at(e.name);
    }
    if (map.param_map.count(e.name)) {
        return map.param_map.at(e.name);
    }
    if (map.const_map.count(e.name)) {
        return map.const_map.at(e.name);
    }
    if (map.bind_map.count(e.name)) {
        return map.bind_map.at(e.name);
    }
    if (map.state_map.count(e.name)) {
        return map.state_map.at(e.name);
    }
    throw std::runtime_error(fmt::format("undefined identifier {}, at {}",
                                         e.name, to_string(e.loc)));
}

// Resolve record aliases first, then parameters, constants, bindings and states.
// Then resolve functions (they need the above to be resolved beforehand).
// Finally, resolve API hooks.
resolved_mechanism resolve(const parsed_mechanism& e) {
    resolved_mechanism mech;
    in_scope_map available_map;

    // Record aliases are used to resolve types then they are dropped.
    // Each record alias has an implicitly defined prime type. e.g.
    // if a `record foo {a:real; b:voltage;};` is defined.
    //    a `record foo' {a':real/time; b':voltage/time;};` is implicitly
    // defined, unless the user explicitly defines it too.
    for (const auto& r: e.records) {
        auto rec = is_parsed_record_alias(r);
        if (!rec) {
            throw std::runtime_error(fmt::format("internal compiler error, expected record expression at {}",
                                                 to_string(location_of(r))));
        }

        // regular
        auto resolved_record = resolve(r, available_map);
        auto resolved_record_val = is_resolved_record_alias(resolved_record);
        if (!resolved_record_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected record expression at {}",
                                                 to_string(location_of(r))));
        }
        if (!available_map.type_map.insert({resolved_record_val->name, resolved_record_val->type}).second) {
            throw std::runtime_error(fmt::format("Record alias `{}` found at {} already previously defined}",
                                                 resolved_record_val->name, to_string(rec->loc)));
        }

        // prime
        auto derived_parsed_record_type = derive(resolved_record_val->type);
        if (derived_parsed_record_type) {
            // if it's already defined, this will fail, which is okay.
            available_map.type_map.insert({resolved_record_val->name+"'", derived_parsed_record_type.value()});
        }
    }
    for (const auto& c: e.constants) {
        auto val = resolve(c, available_map);
        mech.constants.push_back(val);

        auto const_val = is_resolved_constant(val);
        if (!const_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected constant expression at {}",
                                                 to_string(location_of(val))));
        }

        // Add constants to the scope of the entire mechanism as a resolved_argument.
        auto const_arg = make_rexpr<resolved_argument>(const_val->name, const_val->type, const_val->loc);
        if (!available_map.const_map.insert({const_val->name, const_arg}).second) {
            auto found_loc = location_of(available_map.const_map.at(const_val->name));
            throw std::runtime_error(fmt::format("Constant `{}` found at {} already defined at {}",
                                                 const_val->name, to_string(location_of(val)), to_string(found_loc)));
        }
    }
    for (const auto& c: e.parameters) {
        auto val =resolve(c, available_map);
        mech.parameters.push_back(val);

        auto param_val = is_resolved_parameter(val);
        if (!param_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected parameter expression at {}",
                                                 to_string(location_of(val))));
        }

        // Add parameters to the scope of the entire mechanism as a resolved_argument.
        auto param_arg = make_rexpr<resolved_argument>(param_val->name, param_val->type, param_val->loc);
        if (!available_map.param_map.insert({param_val->name, param_arg}).second) {
            auto found_loc = location_of(available_map.param_map.at(param_val->name));
            throw std::runtime_error(fmt::format("Parameter `{}` found at {} already defined at {}",
                                                 param_val->name, to_string(location_of(val)), to_string(found_loc)));
        }
    }
    for (const auto& c: e.bindings) {
        auto val = resolve(c, available_map);
        mech.bindings.push_back(val);

        auto bind_val = is_resolved_bind(val);
        if (!bind_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected bind expression at {}",
                                                 to_string(location_of(val))));
        }

        // Add bindings to the scope of the entire mechanism as a resolved_argument.
        auto bind_arg = make_rexpr<resolved_argument>(bind_val->name, bind_val->type, bind_val->loc);
        if (!available_map.bind_map.insert({bind_val->name, bind_arg}).second) {
            auto found_loc = location_of(available_map.bind_map.at(bind_val->name));
            throw std::runtime_error(fmt::format("Binding `{}` found at {} already defined at {}",
                                                 bind_val->name, to_string(location_of(val)), to_string(found_loc)));
        }
    }
    for (const auto& c: e.states) {
        auto val = resolve(c, available_map);
        mech.states.push_back(val);

        auto state_val = is_resolved_state(val);
        if (!state_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected state expression at {}",
                                                 to_string(location_of(val))));
        }

        // Add states to the scope of the entire mechanism as a resolved_argument.
        auto state_arg = make_rexpr<resolved_argument>(state_val->name, state_val->type, state_val->loc);
        if (!available_map.state_map.insert({state_val->name, state_arg}).second) {
            auto found_loc = location_of(available_map.state_map.at(state_val->name));
            throw std::runtime_error(fmt::format("State `{}` found at {} already defined at {}",
                                                 state_val->name, to_string(location_of(val)), to_string(found_loc)));
        }
    }
    for (const auto& c: e.functions) {
        auto val = resolve(c, available_map);
        mech.functions.push_back(val);

        auto func_val = is_resolved_function(val);
        if (!func_val) {
            throw std::runtime_error(fmt::format("internal compiler error, expected function expression at {}",
                                                 to_string(location_of(val))));
        }

        // Add function expressions to the scope of the entire mechanism.
        if (!available_map.func_map.insert({func_val->name, val}).second) {
            auto found_loc = location_of(available_map.func_map.at(func_val->name));
            throw std::runtime_error(fmt::format("Function `{}` found at {} already defined at {}",
                                                 func_val->name, to_string(location_of(val)), to_string(found_loc)));
        }
    }
    for (const auto& c: e.initializations) {
        mech.initializations.push_back(resolve(c, available_map));
    }
    for (const auto& c: e.on_events) {
        mech.on_events.push_back(resolve(c, available_map));
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

} // namespace al
} // namespace resolved_ir
