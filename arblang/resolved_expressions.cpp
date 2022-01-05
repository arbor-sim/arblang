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
#include <arblang/type_expressions.hpp>

namespace al {
namespace resolved_ir {
using namespace raw_ir;
using namespace t_raw_ir;

// Resolver

void check_duplicate(const std::string& name, const in_scope_map& map) {
    if (map.param_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate parameter name, also found at {}", to_string(map.param_map.at(name).loc)));
    }
    if (map.const_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate parameter name, also found at {}", to_string(map.const_map.at(name).loc)));
    }
    if (map.bind_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate parameter name, also found at {}", to_string(map.bind_map.at(name).loc)));
    }
    if (map.state_map.count(name)) {
        throw std::runtime_error(fmt::format("duplicate parameter name, also found at {}", to_string(map.state_map.at(name).loc)));
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

    if (id.type) {
        return make_rexpr<resolved_parameter>(p_name, p_val, id.type.value(), expr.loc);
    }
    auto p_type = std::visit([&](auto&& c){return c.type;}, *p_val);
    return make_rexpr<resolved_parameter>(p_name, p_val, p_type, expr.loc);
}

r_expr resolve(const raw_ir::constant_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto c_name = id.name;
    check_duplicate(c_name, map);

    auto available_map = map;
    available_map.param_map.clear();
    available_map.bind_map.clear();
    available_map.state_map.clear();

    auto c_val  = std::visit([&](auto&& c){return resolve(c, available_map);}, *(expr.value));

    if (id.type) {
        return make_rexpr<resolved_constant>(c_name, c_val, id.type.value(), expr.loc);
    }
    auto c_type = std::visit([&](auto&& c){return c.type;}, *c_val);
    return make_rexpr<resolved_constant>(c_name, c_val, c_type, expr.loc);
}

r_expr resolve(const raw_ir::state_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto s_name = id.name;
    check_duplicate(s_name, map);

    if (!id.type) {
        throw std::runtime_error(fmt::format("state identifier {} at {} missing quantity type.", s_name, to_string(id.loc)));
    }
    auto s_type = id.type.value();
    return make_rexpr<resolved_state>(s_name, s_type, expr.loc);
}

r_expr resolve(const raw_ir::bind_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto b_name = id.name;
    check_duplicate(b_name, map);

    if (id.type) {
        return make_rexpr<resolved_bind>(b_name, expr.bind, expr.ion, id.type.value(), expr.loc);
    }
    auto b_type = type_of(expr.bind, expr.loc);
    return make_rexpr<resolved_bind>(b_name, expr.bind, expr.ion, b_type, expr.loc);
}

r_expr resolve(const raw_ir::record_alias_expr& expr, const in_scope_map& map) {
    return make_rexpr<resolved_record_alias>(expr.name, expr.type, expr.loc);
}

r_expr resolve(const raw_ir::function_expr& expr, const in_scope_map& map) {
    auto f_name = expr.name;
    if (map.func_map.count(f_name)) {
        throw std::runtime_error(fmt::format("duplicate function name, also found at {}", to_string(map.param_map.at(f_name).loc)));
    }
    auto available_map = map;
    std::vector<r_expr> f_args;
    for (const auto& a: expr.args) {
        auto a_id = std::get<identifier_expr>(*a);
        if (!a_id.type) {
            throw std::runtime_error(fmt::format("function argument {} at {} missing quantity type.", a_id.name, to_string(a_id.loc)));
        }
        auto f_a = resolved_argument(a_id.name, a_id.type.value(), a_id.loc);
        f_args.push_back(make_rexpr<resolved_argument>(f_a));
        available_map.local_map.insert({a_id.name, f_a});
    }
    auto f_body = std::visit([&](auto&& c){return resolve(c, available_map);}, *expr.body);

    if (expr.ret) {
        return make_rexpr<resolved_function>(f_name, f_args, f_body, expr.ret.value(), expr.loc);
    }
    auto f_type = std::visit([](auto&& c){return c.type;}, *f_body);
    return make_rexpr<resolved_function>(f_name, f_args, f_body, f_type, expr.loc);
}

r_expr resolve(const raw_ir::initial_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto i_name = id.name;

    if (!map.state_map.count(i_name)) {
        throw std::runtime_error(fmt::format("variable {} initialized at {} is not a state variable.", i_name, to_string(expr.loc)));
    }
    auto i_expr = make_rexpr<resolved_state>(map.state_map.at(i_name));
    auto i_val = std::visit([&](auto&& c){return resolve(c, map);}, *(expr.value));

    if (id.type) {
        return make_rexpr<resolved_initial>(i_expr, i_val, id.type.value(), expr.loc);
    }
    auto i_type = std::visit([&](auto&& c){return c.type;}, *i_val);
    return make_rexpr<resolved_initial>(i_expr, i_val, i_type, expr.loc);
}

r_expr resolve(const raw_ir::evolve_expr& expr, const in_scope_map& map) {
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

    if (id.type) {
        return make_rexpr<resolved_evolve>(e_expr, e_val, id.type.value(), expr.loc);
    }
    auto e_type = std::visit([&](auto&& c){return c.type;}, *e_val);
    return make_rexpr<resolved_evolve>(e_expr, e_val, e_type, expr.loc);
}

r_expr resolve(const raw_ir::effect_expr& expr, const in_scope_map& map) {
    auto e_effect = expr.effect;
    auto e_ion = expr.ion;
    auto e_val = std::visit([&](auto&& c){return resolve(c, map);}, *(expr.value));
    auto e_type = type_of(e_effect, expr.loc);

    return make_rexpr<resolved_effect>(e_effect, e_ion, e_val, e_type, expr.loc);
}

r_expr resolve(const raw_ir::export_expr& expr, const in_scope_map& map) {
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto p_name = id.name;

    if (!map.param_map.count(p_name)) {
        throw std::runtime_error(fmt::format("variable {} exported at {} is not a parameter.", p_name, to_string(expr.loc)));
    }
    auto p_expr = map.param_map.at(p_name);
    return make_rexpr<resolved_export>(make_rexpr<resolved_parameter>(p_expr), p_expr.type, expr.loc);
}

r_expr resolve(const raw_ir::call_expr& expr, const in_scope_map& map) {
    auto f_name = expr.function_name;
    if (!map.func_map.count(f_name)) {
        throw std::runtime_error(fmt::format("function {} called at {} is not defined.", f_name, to_string(expr.loc)));
    }
    auto f_expr = map.func_map.at(f_name);

    std::vector<r_expr> c_args;
    for (const auto& a: expr.call_args) {
        c_args.push_back(std::visit([&](auto&& c){return resolve(c, map);}, *a));
    }
    return make_rexpr<resolved_call>(make_rexpr<resolved_function>(f_expr), c_args, f_expr.type, expr.loc);
}

r_expr resolve(const raw_ir::object_expr& expr, const in_scope_map& map) {
    assert(expr.record_fields.size() == expr.record_values.size());
    std::vector<r_expr> o_values;
    std::vector<r_expr> o_fields;
    std::vector<t_expr> o_types;

    for (const auto& v: expr.record_values) {
        o_values.push_back(std::visit([&](auto&& c){return resolve(c, map);}, *v));
        o_types.push_back(std::visit([&](auto&& c){return c.type;}, *o_values.back()));
    }
    std::vector<std::pair<std::string, t_expr>> t_vec;
    for (unsigned i = 0; i < expr.record_fields.size(); ++i) {
        auto f_id = std::get<identifier_expr>(*expr.record_fields[i]);
        o_fields.push_back(make_rexpr<resolved_argument>(f_id.name, o_types[i], f_id.loc));
        t_vec.emplace_back(f_id.name, o_types[i]);
    }
    auto o_type = make_t_expr<record_type>(t_vec, expr.loc);

    if (!expr.record_name) {
        return make_rexpr<resolved_object>(std::nullopt, o_fields, o_values, o_type, expr.loc);
    }

    auto r_name = expr.record_name.value();
    if (!map.rec_map.count(r_name)) {
        throw std::runtime_error(fmt::format("record {} referenced at {} is not defined.", r_name, to_string(expr.loc)));
    }
    auto rec_expr = map.rec_map.at(r_name);
    return make_rexpr<resolved_object>(make_rexpr<resolved_record_alias>(rec_expr), o_fields, o_values, rec_expr.type, expr.loc);
}

r_expr resolve(const raw_ir::let_expr& expr, const in_scope_map& map) {
    auto v_expr = std::visit([&](auto&& c){return resolve(c, map);}, *expr.value);
    auto v_type = std::visit([&](auto&& c){return c.type;}, *v_expr);
    auto id = std::get<identifier_expr>(*expr.identifier);
    auto i_expr = id.type? resolved_argument(id.name, id.type.value(), id.loc): resolved_argument(id.name, v_type, id.loc);

    auto available_map = map;
    available_map.local_map.insert({id.name, i_expr});

    auto b_expr = std::visit([&](auto&& c){return resolve(c, available_map);}, *expr.body);
    auto b_type = std::visit([&](auto&& c){return c.type;}, *b_expr);

    return make_rexpr<resolved_let>(make_rexpr<resolved_argument>(i_expr), v_expr, b_expr, b_type, expr.loc);
}

r_expr resolve(const raw_ir::with_expr& expr, const in_scope_map& map) {
    auto v_expr = std::visit([&](auto&& c){return resolve(c, map);}, *expr.value);
    auto v_type = std::visit([&](auto&& c){return c.type;}, *v_expr);

    auto available_map = map;
    if (std::get_if<record_type>(v_type.get()) || std::get_if<record_alias_type>(v_type.get())) {
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

r_expr resolve(const raw_ir::conditional_expr& expr, const in_scope_map& map) {
    return {};
}

r_expr resolve(const raw_ir::float_expr& expr, const in_scope_map& map) {
    auto f_val = expr.value;
    auto f_type = std::visit([&](auto&& c){return u_raw_ir::to_type(c);}, *expr.unit);
    return make_rexpr<resolved_float>(f_val, f_type, expr.loc);
}

r_expr resolve(const raw_ir::int_expr& expr, const in_scope_map& map) {
    auto i_val = expr.value;
    auto i_type = std::visit([&](auto&& c){return u_raw_ir::to_type(c);}, *expr.unit);
    return make_rexpr<resolved_int>(i_val, i_type, expr.loc);
}

r_expr resolve(const raw_ir::unary_expr& expr, const in_scope_map& map) {
    switch (expr.op) {
        case unary_op::exp:
        case unary_op::log:
        case unary_op::cos:
        case unary_op::sin:
        case unary_op::exprelr:
        case unary_op::lnot: {
            auto val = std::visit([&](auto &&c) { return to_string(c); }, *expr.value);
            throw std::runtime_error(
                    fmt::format("Cannot apply operator {} to expr {} because it is not a \"real\" quantity type.",
                                to_string(expr.op), val));
        }
        default: break;
    }
    auto u_val = std::visit([&](auto&& c){return resolve(c, map);}, *expr.value);
//    return make_rexpr<resolved_unary>();
}

r_expr resolve(const raw_ir::binary_expr& expr, const in_scope_map& map) {return {};}

r_expr resolve(const raw_ir::identifier_expr& expr, const in_scope_map& map) {return {};}

r_expr resolve(const raw_ir::mechanism_expr&, const in_scope_map&) {return {};}

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
    for (const auto& p: e.records) {
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
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_constant
std::string to_string(const resolved_constant& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_constant\n";
    str += (double_indent + e.name + "\n");
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_state
std::string to_string(const resolved_state& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_state\n";
    str += (double_indent + e.name + "\n");
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_record_alias
std::string to_string(const resolved_record_alias& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_record_alias\n";
    str += (double_indent + e.name + "\n");
    std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *(e.type));
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
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_evolve
std::string to_string(const resolved_evolve& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_evolve\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
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
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_export
std::string to_string(const resolved_export& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_export\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
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
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_with
std::string to_string(const resolved_with& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";
    std::string str = single_indent + "(resolved_with\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.body);
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
    return str + double_indent + to_string(e.loc) + ")";
}

// resolved_binary
std::string to_string(const resolved_binary& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_binary " + to_string(e.op) + "\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.lhs);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.rhs);
    return str + double_indent + to_string(e.loc) + ")";
}

} // namespace al
} // namespace raw_ir