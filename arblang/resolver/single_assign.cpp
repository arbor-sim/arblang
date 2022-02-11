#include <string>
#include <unordered_set>

#include <arblang/resolver/single_assign.hpp>
#include <arblang/util/unique_name.hpp>

namespace al {
namespace resolved_ir {

// TODO make sure that canonicalize is called before single_assign
resolved_mechanism single_assign(const resolved_mechanism&);

r_expr single_assign(const resolved_parameter& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    if(!reserved.insert(e.name).second) {
        throw std::runtime_error("Parameter "+e.name+" at "+to_string(e.loc)+
                                 " shadows another parameter, constant, state variable or binding, with the same name");
    }
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    return make_rexpr<resolved_parameter>(e.name, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_constant& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    if(!reserved.insert(e.name).second) {
        throw std::runtime_error("Parameter "+e.name+" at "+to_string(e.loc)+
                                 " shadows another parameter, constant, state variable or binding, with the same name");
    }
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    return make_rexpr<resolved_constant>(e.name, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_state& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    if(!reserved.insert(e.name).second) {
        throw std::runtime_error("State "+e.name+" at "+to_string(e.loc)+
                                 " shadows another parameter, constant, state variable or binding, with the same name");
    }
    return make_rexpr<resolved_state>(e);
}

r_expr single_assign(const resolved_record_alias& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    // TODO check there are no duplicate record alias names
    return make_rexpr<resolved_record_alias>(e);
}

r_expr single_assign(const resolved_function& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    // TODO check there are no duplicate function names?
    auto body_ssa = single_assign(e, reserved, rewrites);
    return make_rexpr<resolved_function>(e.name, e.args, body_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_argument& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    if (rewrites.count(e.name)) {
        auto arg = resolved_argument(rewrites[e.name], e.type, e.loc);
        return make_rexpr<resolved_argument>(rewrites[e.name], e.type, e.loc);
    }
    return make_rexpr<resolved_argument>(e);
}

r_expr single_assign(const resolved_bind& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    if(!reserved.insert(e.name).second) {
        throw std::runtime_error("Binding "+e.name+" at "+to_string(e.loc)+
                                 " shadows another parameter, constant, state variable or binding, with the same name");
    }
    return make_rexpr<resolved_bind>(e);
}

r_expr single_assign(const resolved_initial& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    return make_rexpr<resolved_initial>(e.identifier, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_evolve& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    return make_rexpr<resolved_evolve>(e.identifier, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_effect& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    return make_rexpr<resolved_effect>(e.effect, e.ion, val_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_export& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    return make_rexpr<resolved_export>(e);
}

r_expr single_assign(const resolved_call& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    std::vector<r_expr> args_ssa;
    for (const auto& a: e.call_args) {
        args_ssa.push_back(single_assign(a, reserved, rewrites));
    }
    return make_rexpr<resolved_call>(e.f_identifier, args_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_object& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    std::vector<r_expr> fields_ssa, args_ssa;
    for (const auto& a: e.record_fields) {
        fields_ssa.push_back(single_assign(a, reserved, rewrites));
    }
    for (const auto& a: e.record_values) {
        args_ssa.push_back(single_assign(a, reserved, rewrites));
    }
    return make_rexpr<resolved_object>(e.r_identifier, fields_ssa, args_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_let& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    auto val_ssa = single_assign(e.value, reserved, rewrites);
    auto iden_ssa = e.identifier;

    auto iden = std::get<resolved_argument>(*e.identifier);
    if (!reserved.insert(iden.name).second) {
        auto rename = unique_local_name(reserved, "r");
        rewrites[iden.name] = rename;
        reserved.insert(rename);
        iden_ssa = make_rexpr<resolved_argument>(rename, iden.type, iden.loc);
    }

    auto body_ssa = single_assign(e.body, reserved, rewrites);

    return make_rexpr<resolved_let>(iden_ssa, val_ssa, body_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_conditional& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    auto cond_ssa = single_assign(e.condition, reserved, rewrites);
    auto tval_ssa = single_assign(e.value_true, reserved, rewrites);
    auto fval_ssa = single_assign(e.value_false, reserved, rewrites);

    return make_rexpr<resolved_conditional>(cond_ssa, tval_ssa, fval_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_float& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    return make_rexpr<resolved_float>(e);
}

r_expr single_assign(const resolved_int& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    return make_rexpr<resolved_int>(e);
}

r_expr single_assign(const resolved_unary& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    auto arg_ssa = single_assign(e.arg, reserved, rewrites);
    return make_rexpr<resolved_unary>(e.op, arg_ssa, e.type, e.loc);
}

r_expr single_assign(const resolved_binary& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    auto lhs_ssa = single_assign(e.lhs, reserved, rewrites);
    auto rhs_ssa = e.op == binary_op::dot? e.rhs: single_assign(e.rhs, reserved, rewrites);
    return make_rexpr<resolved_binary>(e.op, lhs_ssa, rhs_ssa, e.type, e.loc);
}

r_expr single_assign(const r_expr& e,
                     std::unordered_set<std::string>& reserved,
                     std::unordered_map<std::string, std::string>& rewrites)
{
    return std::visit([&](auto& c) {return single_assign(c, reserved, rewrites);}, *e);
}

r_expr single_assign(const r_expr& e) {
    std::unordered_set<std::string> reserved;
    std::unordered_map<std::string, std::string> rewrites;
    return single_assign(e, reserved, rewrites);
}

} // namespace resolved_ir
} // namespace al