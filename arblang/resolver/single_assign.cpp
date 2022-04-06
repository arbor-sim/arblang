#include <string>
#include <unordered_set>

#include <arblang/resolver/single_assign.hpp>
#include <arblang/util/unique_name.hpp>

namespace al {
namespace resolved_ir {
// Single assignment is the process of rewriting expressions such that
// no 2 variables share the same name.
// Before this step, the following is possible:
//   let a = 4;
//   let a = 7.3;
//   let b = a;
// We want to change this to:
//   let a = 4;
//   let _r0 = 7.3;
//   let b = _r0;
// The following is also possible:
//   parameter a = ...
//   ...
//   let a = ..
// We want to change this to :
//   parameter a = ...
//   ...
//   let _t0 = ...
// Single assignment is a prerequisite for the optimization passes.

r_expr single_assign(const resolved_record_alias& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation.");
}

r_expr single_assign(const resolved_argument& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    return make_rexpr<resolved_argument>(e);
}

// Resolved variables may be rewritten similar to the
// canonicalization process.
r_expr single_assign(const resolved_variable& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    if (rewrites.count(e.name)) {
        return rewrites[e.name];
    }
    return make_rexpr<resolved_variable>(e);
}

r_expr single_assign(const resolved_parameter& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_parameter>(e.name, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_constant& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_constant>(e.name, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_state& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    return make_rexpr<resolved_state>(e);
}

r_expr single_assign(const resolved_function& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto body_ssa = single_assign(e.body, reserved, rewrites, pref);
    return make_rexpr<resolved_function>(e.name, e.args, body_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_bind& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    return make_rexpr<resolved_bind>(e);
}

r_expr single_assign(const resolved_initial& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_initial>(e.identifier, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_on_event& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_on_event>(e.argument, e.identifier, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_evolve& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_evolve>(e.identifier, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_effect& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_effect>(e.effect, e.ion, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_export& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    return make_rexpr<resolved_export>(e);
}

r_expr single_assign(const resolved_call& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    std::vector<r_expr> args_ssa;
    for (const auto& a: e.call_args) {
        args_ssa.push_back(single_assign(a, reserved, rewrites, pref));
    }
    return make_rexpr<resolved_call>(e.f_identifier, args_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_object& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    std::vector<r_expr> fields_ssa;
    for (const auto& a: e.field_values()) {
        fields_ssa.push_back(single_assign(a, reserved, rewrites, pref));
    }
    return make_rexpr<resolved_object>(e.field_names(), fields_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_let& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto val_ssa = single_assign(e.id_value(), reserved, rewrites, pref);
    r_expr var_ssa;

    auto var_name = e.id_name();
    if (!reserved.insert(var_name).second) {
        var_name = unique_local_name(reserved, pref);
    }
    var_ssa = make_rexpr<resolved_variable>(var_name, val_ssa, type_of(val_ssa), location_of(val_ssa));
    rewrites[e.id_name()] = var_ssa;

    auto body_ssa = single_assign(e.body, reserved, rewrites, pref);
    return make_rexpr<resolved_let>(var_ssa, body_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_conditional& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto cond_ssa = single_assign(e.condition, reserved, rewrites, pref);
    auto tval_ssa = single_assign(e.value_true, reserved, rewrites, pref);
    auto fval_ssa = single_assign(e.value_false, reserved, rewrites, pref);

    return make_rexpr<resolved_conditional>(cond_ssa, tval_ssa, fval_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_float& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    return make_rexpr<resolved_float>(e);
}

r_expr single_assign(const resolved_int& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    return make_rexpr<resolved_int>(e);
}

r_expr single_assign(const resolved_unary& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto arg_ssa = single_assign(e.arg, reserved, rewrites, pref);
    return make_rexpr<resolved_unary>(e.op, arg_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_binary& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto lhs_ssa = single_assign(e.lhs, reserved, rewrites, pref);
    auto rhs_ssa = single_assign(e.rhs, reserved, rewrites, pref);
    return make_rexpr<resolved_binary>(e.op, lhs_ssa, rhs_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_field_access& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    auto obj_ssa = single_assign(e.object, reserved, rewrites, pref);
    return make_rexpr<resolved_field_access>(obj_ssa, e.field, e.type, e.loc);
}

// TODO make sure that canonicalize is called before single_assign
resolved_mechanism single_assign(const resolved_mechanism& e) {
    std::unordered_set<std::string> globals;
    std::unordered_set<std::string> reserved;
    std::unordered_map<std::string, r_expr> rewrites;
    std::string pref = "r";
    resolved_mechanism mech;

    // Get all globally available symbols and mark them as reserved.
    for (const auto& c: e.constants) {
        globals.insert(is_resolved_constant(c)->name);
    }
    for (const auto& c: e.parameters) {
        globals.insert(is_resolved_parameter(c)->name);
    }
    for (const auto& c: e.bindings) {
        globals.insert(is_resolved_bind(c)->name);
    }
    for (const auto& c: e.states) {
        globals.insert(is_resolved_state(c)->name);
    }

    // Handle expressions
    for (const auto& c: e.constants) {
        reserved = globals;
        mech.constants.push_back(single_assign(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.bindings) {
        reserved = globals;
        rewrites.clear();
        mech.bindings.push_back(single_assign(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.states) {
        reserved = globals;
        rewrites.clear();
        mech.states.push_back(single_assign(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.functions) {
        reserved = globals;
        rewrites.clear();
        mech.functions.push_back(single_assign(c, reserved, rewrites, pref));
    }

    // All parameters and initializations share the same reserved_map
    // because they share the same API call.
    reserved = globals;
    for (const auto& c: e.parameters) {
        rewrites.clear();
        mech.parameters.push_back(single_assign(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.initializations) {
        rewrites.clear();
        mech.initializations.push_back(single_assign(c, reserved, rewrites, pref));
    }

    // All on_events share the same reserved_map because they share the
    // same API call.
    reserved = globals;
    for (const auto& c: e.on_events) {
        rewrites.clear();
        mech.on_events.push_back(single_assign(c, reserved, rewrites, pref));
    }

    // All evolutions share the same reserved_map because they share the
    // same API call.
    reserved = globals;
    for (const auto& c: e.evolutions) {
        rewrites.clear();
        mech.evolutions.push_back(single_assign(c, reserved, rewrites, pref));
    }

    // All effects share the same reserved_map because they share the
    // same API call.
    reserved = globals;
    for (const auto& c: e.effects) {
        rewrites.clear();
        mech.effects.push_back(single_assign(c, reserved, rewrites, pref));
    }

    reserved = globals;
    for (const auto& c: e.exports) {
        reserved = globals;
        rewrites.clear();
        mech.exports.push_back(single_assign(c, reserved, rewrites, pref));
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return mech;
}

r_expr single_assign(const r_expr& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites,
                     const std::string& pref)
{
    return std::visit([&](auto& c) {return single_assign(c, reserved, rewrites, pref);}, *e);
}

r_expr single_assign(const r_expr& e, const std::string& pref) {
    std::unordered_set<std::string> reserved;
    std::unordered_map<std::string, r_expr> rewrites;
    return single_assign(e, reserved, rewrites, pref);
}

} // namespace resolved_ir
} // namespace al