#include <string>
#include <unordered_set>

#include <arblang/resolver/single_assign.hpp>
#include <arblang/util/unique_name.hpp>

namespace al {
namespace resolved_ir {

// TODO make sure that canonicalize is called before single_assign
resolved_mechanism single_assign(const resolved_mechanism& e) {
    std::unordered_set<std::string> globals;
    std::unordered_set<std::string> reserved;
    std::unordered_map<std::string, r_expr> rewrites;
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

    // Handle expressions
    for (const auto& c: e.constants) {
        reserved = globals;
        mech.constants.push_back(single_assign(c, reserved, rewrites));
    }
    for (const auto& c: e.parameters) {
        reserved = globals;
        rewrites.clear();
        mech.parameters.push_back(single_assign(c, reserved, rewrites));
    }
    for (const auto& c: e.bindings) {
        reserved = globals;
        rewrites.clear();
        mech.bindings.push_back(single_assign(c, reserved, rewrites));
    }
    for (const auto& c: e.states) {
        reserved = globals;
        rewrites.clear();
        mech.states.push_back(single_assign(c, reserved, rewrites));
    }
    for (const auto& c: e.functions) {
        reserved = globals;
        rewrites.clear();
        mech.functions.push_back(single_assign(c, reserved, rewrites));
    }
    for (const auto& c: e.initializations) {
        reserved = globals;
        rewrites.clear();
        mech.initializations.push_back(single_assign(c, reserved, rewrites));
    }
    for (const auto& c: e.evolutions) {
        reserved = globals;
        rewrites.clear();
        mech.evolutions.push_back(single_assign(c, reserved, rewrites));
    }
    for (const auto& c: e.effects) {
        reserved = globals;
        rewrites.clear();
        mech.effects.push_back(single_assign(c, reserved, rewrites));
    }
    for (const auto& c: e.exports) {
        reserved = globals;
        rewrites.clear();
        mech.exports.push_back(single_assign(c, reserved, rewrites));
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return mech;
}

r_expr single_assign(const resolved_record_alias& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation.");
}

r_expr single_assign(const resolved_argument& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    if (rewrites.count(e.name)) {
        return rewrites[e.name];
    }
    return make_rexpr<resolved_argument>(e);
}

r_expr single_assign(const resolved_variable& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    if (rewrites.count(e.name)) {
        return rewrites[e.name];
    }
    return make_rexpr<resolved_variable>(e);
}

r_expr single_assign(const resolved_parameter& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    return make_rexpr<resolved_parameter>(e.name, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_constant& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    return make_rexpr<resolved_constant>(e.name, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_state& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    return make_rexpr<resolved_state>(e);
}

r_expr single_assign(const resolved_function& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto body_ssa = single_assign(e.body, reserved, rewrites);
    return make_rexpr<resolved_function>(e.name, e.args, body_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_bind& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    return make_rexpr<resolved_bind>(e);
}

r_expr single_assign(const resolved_initial& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    return make_rexpr<resolved_initial>(e.identifier, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_evolve& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    return make_rexpr<resolved_evolve>(e.identifier, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_effect& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    return make_rexpr<resolved_effect>(e.effect, e.ion, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_export& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    return make_rexpr<resolved_export>(e);
}

r_expr single_assign(const resolved_call& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    std::vector<r_expr> args_ssa;
    for (const auto& a: e.call_args) {
        args_ssa.push_back(single_assign(a, reserved, rewrites));
    }
    return make_rexpr<resolved_call>(e.f_identifier, args_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_object& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    std::vector<r_expr> fields_ssa, args_ssa;
    for (const auto& a: e.record_fields) {
        fields_ssa.push_back(single_assign(a, reserved, rewrites));
    }
    for (const auto& a: e.record_values) {
        args_ssa.push_back(single_assign(a, reserved, rewrites));
    }
    return make_rexpr<resolved_object>(fields_ssa, args_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_let& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    auto iden_ssa = e.identifier;

    auto iden = std::get<resolved_variable>(*e.identifier);
    if (!reserved.insert(iden.name).second) {
        auto rename = unique_local_name(reserved, "r");
        iden_ssa = make_rexpr<resolved_variable>(rename, iden.value, iden.type, iden.loc);
        rewrites[iden.name] = iden_ssa;
    }

    auto body_ssa = single_assign(e.body, reserved, rewrites);

    return make_rexpr<resolved_let>(iden_ssa, val_ssa, body_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_conditional& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto cond_ssa = single_assign(e.condition, reserved, rewrites);
    auto tval_ssa = single_assign(e.value_true, reserved, rewrites);
    auto fval_ssa = single_assign(e.value_false, reserved, rewrites);

    return make_rexpr<resolved_conditional>(cond_ssa, tval_ssa, fval_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_float& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    return make_rexpr<resolved_float>(e);
}

r_expr single_assign(const resolved_int& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    return make_rexpr<resolved_int>(e);
}

r_expr single_assign(const resolved_unary& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto arg_ssa = single_assign(e.arg, reserved, rewrites);
    return make_rexpr<resolved_unary>(e.op, arg_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_binary& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto lhs_ssa = single_assign(e.lhs, reserved, rewrites);
    auto rhs_ssa = single_assign(e.rhs, reserved, rewrites);
    return make_rexpr<resolved_binary>(e.op, lhs_ssa, rhs_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_field_access& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    auto obj_ssa = single_assign(e.object, reserved, rewrites);
    return make_rexpr<resolved_field_access>(obj_ssa, e.field, e.type, e.loc);
}

r_expr single_assign(const r_expr& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, r_expr>& rewrites)
{
    return std::visit([&](auto& c) {return single_assign(c, reserved, rewrites);}, *e);
}

r_expr single_assign(const r_expr& e) {
    std::unordered_set<std::string> reserved;
    std::unordered_map<std::string, r_expr> rewrites;
    return single_assign(e, reserved, rewrites);
}

} // namespace resolved_ir
} // namespace al