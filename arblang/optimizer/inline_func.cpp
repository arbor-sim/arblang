#include <string>
#include <unordered_set>

#include <arblang/optimizer/inline_func.hpp>
#include <arblang/util/unique_name.hpp>

#include "../util/rexp_helpers.hpp"

namespace al {
namespace resolved_ir {

resolved_mechanism inline_func(const resolved_mechanism& e) {
    std::unordered_set<std::string> globals;
    std::unordered_set<std::string> reserved;
    std::unordered_map<std::string, r_expr> rewrites, avail_funcs;
    resolved_mechanism mech;

    // Get all globally available symbols
    for (const auto& c: e.constants) {
        globals.insert(std::get<resolved_constant>(*c).name);
    }
    for (const auto& c: e.parameters) {
        globals.insert(std::get<resolved_parameter>(*c).name);
    }
    for (const auto& c: e.bindings) {
        globals.insert(std::get<resolved_bind>(*c).name);
    }
    for (const auto& c: e.states) {
        globals.insert(std::get<resolved_state>(*c).name);
    }

    // Get all globally available functions
    for (const auto& c: e.functions) {
        avail_funcs.insert({std::get<resolved_function>(*c).name, c});
    }

    for (const auto& c: e.constants) {
        reserved = globals;
        rewrites.clear();
        mech.constants.push_back(inline_func(c, reserved, rewrites, avail_funcs));
    }
    for (const auto& c: e.parameters) {
        reserved = globals;
        rewrites.clear();
        mech.parameters.push_back(inline_func(c, reserved, rewrites, avail_funcs));
    }
    for (const auto& c: e.bindings) {
        reserved = globals;
        rewrites.clear();
        mech.bindings.push_back(inline_func(c, reserved, rewrites, avail_funcs));
    }
    for (const auto& c: e.states) {
        reserved = globals;
        rewrites.clear();
        mech.states.push_back(inline_func(c, reserved, rewrites, avail_funcs));
    }
    for (const auto& c: e.initializations) {
        reserved = globals;
        rewrites.clear();
        mech.initializations.push_back(inline_func(c, reserved, rewrites, avail_funcs));
    }
    for (const auto& c: e.evolutions) {
        reserved = globals;
        rewrites.clear();
        mech.evolutions.push_back(inline_func(c, reserved, rewrites, avail_funcs));
    }
    for (const auto& c: e.effects) {
        reserved = globals;
        rewrites.clear();
        mech.effects.push_back(inline_func(c, reserved, rewrites, avail_funcs));
    }
    for (const auto& c: e.exports) {
        reserved = globals;
        rewrites.clear();
        mech.exports.push_back(inline_func(c, reserved, rewrites, avail_funcs));
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return mech;
}

r_expr inline_func(const resolved_parameter& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    auto val_ssa = inline_func(e.value, reserved, rewrites, avail_funcs);
    return make_rexpr<resolved_parameter>(e.name, val_ssa, e.type, e.loc);
}

r_expr inline_func(const resolved_constant& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    auto val_ssa = inline_func(e.value, reserved, rewrites, avail_funcs);
    return make_rexpr<resolved_constant>(e.name, val_ssa, e.type, e.loc);
}

r_expr inline_func(const resolved_state& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    return make_rexpr<resolved_state>(e);
}

r_expr inline_func(const resolved_record_alias& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    return make_rexpr<resolved_record_alias>(e);
}

r_expr inline_func(const resolved_function& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    auto body_ssa = inline_func(e.body, reserved, rewrites, avail_funcs);
    return make_rexpr<resolved_function>(e.name, e.args, body_ssa, e.type, e.loc);
}

r_expr inline_func(const resolved_argument& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    if (rewrites.count(e.name)) {
        return rewrites[e.name];
    }
    return make_rexpr<resolved_argument>(e);
}

r_expr inline_func(const resolved_bind& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    return make_rexpr<resolved_bind>(e);
}

r_expr inline_func(const resolved_initial& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    auto val_ssa = inline_func(e.value, reserved, rewrites, avail_funcs);
    return make_rexpr<resolved_initial>(e.identifier, val_ssa, e.type, e.loc);
}

r_expr inline_func(const resolved_evolve& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    auto val_ssa = inline_func(e.value, reserved, rewrites, avail_funcs);
    return make_rexpr<resolved_evolve>(e.identifier, val_ssa, e.type, e.loc);
}

r_expr inline_func(const resolved_effect& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    auto val_ssa = inline_func(e.value, reserved, rewrites, avail_funcs);
    return make_rexpr<resolved_effect>(e.effect, e.ion, val_ssa, e.type, e.loc);
}

r_expr inline_func(const resolved_export& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    return make_rexpr<resolved_export>(e);
}

r_expr inline_func(const resolved_call& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    std::vector<r_expr> args_ssa;
    for (const auto& a: e.call_args) {
        args_ssa.push_back(inline_func(a, reserved, rewrites, avail_funcs));
    }

    // inline function call

    // Get the function from available functions
    auto func = avail_funcs.at(e.f_identifier);

    // Set up f_rewrites to replace the function arguments with the call arguments
    int idx = 0;
    std::unordered_map<std::string, r_expr> f_rewrites;
    for (const auto& a: std::get<resolved_function>(*func).args) {
        f_rewrites.insert({std::get<resolved_argument>(*a).name, args_ssa[idx++]});
    }

    // Set up f_avail_funcs to disallow recursion
    std::unordered_map<std::string, r_expr> f_avail_funcs;
    f_avail_funcs = avail_funcs;
    f_avail_funcs.erase(e.f_identifier);

    // Keep the same reserved values to ensure that the inlined body of the function doesn't
    // overwrite anything from the surrounded context
    auto func_ssa = inline_func(func, reserved, f_rewrites, f_avail_funcs);

    // body of the function can be directly inlined
    return std::get<resolved_function>(*func_ssa).body;
}

r_expr inline_func(const resolved_object& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    std::vector<r_expr> fields_ssa, args_ssa;
    for (const auto& a: e.record_fields) {
        fields_ssa.push_back(inline_func(a, reserved, rewrites, avail_funcs));
    }
    for (const auto& a: e.record_values) {
        args_ssa.push_back(inline_func(a, reserved, rewrites, avail_funcs));
    }
    return make_rexpr<resolved_object>(e.r_identifier, fields_ssa, args_ssa, e.type, e.loc);
}

r_expr inline_func(const resolved_let& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    auto val_ssa = inline_func(e.value, reserved, rewrites, avail_funcs);
    auto iden_ssa = e.identifier;

    auto iden = std::get<resolved_argument>(*e.identifier);
    if (!reserved.insert(iden.name).second) {
        auto rename = unique_local_name(reserved, "r");
        reserved.insert(rename);
        iden_ssa = make_rexpr<resolved_argument>(rename, iden.type, iden.loc);
        rewrites[iden.name] = iden_ssa;
    }

    auto body_ssa = inline_func(e.body, reserved, rewrites, avail_funcs);

    // Extract let
    auto let_outer = resolved_let(iden_ssa, val_ssa, body_ssa, e.type, e.loc);
    if (auto let_first = std::get_if<resolved_let>(val_ssa.get())) {
        let_outer.value = get_innermost_body(let_first);
        set_innermost_body(let_first, make_rexpr<resolved_let>(let_outer));
        return make_rexpr<resolved_let>(*let_first);
    }
    return make_rexpr<resolved_let>(let_outer);
}

r_expr inline_func(const resolved_conditional& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    auto cond_ssa = inline_func(e.condition, reserved, rewrites, avail_funcs);
    auto tval_ssa = inline_func(e.value_true, reserved, rewrites, avail_funcs);
    auto fval_ssa = inline_func(e.value_false, reserved, rewrites, avail_funcs);

    return make_rexpr<resolved_conditional>(cond_ssa, tval_ssa, fval_ssa, e.type, e.loc);
}

r_expr inline_func(const resolved_float& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    return make_rexpr<resolved_float>(e);
}

r_expr inline_func(const resolved_int& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    return make_rexpr<resolved_int>(e);
}

r_expr inline_func(const resolved_unary& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    auto arg_ssa = inline_func(e.arg, reserved, rewrites, avail_funcs);
    return make_rexpr<resolved_unary>(e.op, arg_ssa, e.type, e.loc);
}

r_expr inline_func(const resolved_binary& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    auto lhs_ssa = inline_func(e.lhs, reserved, rewrites, avail_funcs);
    auto rhs_ssa = e.op == binary_op::dot? e.rhs: inline_func(e.rhs, reserved, rewrites, avail_funcs);
    return make_rexpr<resolved_binary>(e.op, lhs_ssa, rhs_ssa, e.type, e.loc);
}

r_expr inline_func(const r_expr& e,
                   std::unordered_set<std::string>& reserved,
                   std::unordered_map<std::string, r_expr>& rewrites,
                   std::unordered_map<std::string, r_expr>& avail_funcs)
{
    return std::visit([&](auto& c) {return inline_func(c, reserved, rewrites, avail_funcs);}, *e);
}

r_expr inline_func(const r_expr& e, std::unordered_map<std::string, r_expr>& avail_funcs) {
    std::unordered_set<std::string> reserved;
    std::unordered_map<std::string, r_expr> rewrites;
    return inline_func(e, reserved, rewrites, avail_funcs);
}

} // namespace resolved_ir
} // namespace al