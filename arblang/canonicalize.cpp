#include <string>
#include <unordered_set>

#include <arblang/canonicalize.hpp>

namespace al {
namespace resolved_ir {

static std::string unique_local_name(std::unordered_set<std::string>& reserved, std::string const& prefix = "t") {
    for (int i = 0; ; ++i) {
        std::string name = prefix + std::to_string(i) + "_";
        if (reserved.insert(name).second) {
            return name;
        }
    }
}

r_expr get_innermost_body(resolved_let* let) {
    resolved_let* let_last = let;
    while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
        let_last = let_next;
    }
    return let_last->body;
}

void set_innermost_body(resolved_let* let, const r_expr& body) {
    auto body_type = type_of(body);
    resolved_let* let_last = let;
    let_last->type = body_type;
    while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
        let_last = let_next;
        let_last->type = body_type;
    }
    let_last->body = body;
}

// Canonicalize
resolved_mechanism canonicalize(const resolved_mechanism&);

r_expr canonicalize(const resolved_parameter& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    return make_rexpr<resolved_parameter>(e.name, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_constant& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    return make_rexpr<resolved_constant>(e.name, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_state& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_state>(e);
}

r_expr canonicalize(const resolved_record_alias& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_record_alias>(e);
}

r_expr canonicalize(const resolved_function& e, std::unordered_set<std::string>& reserved) {
    auto body_canon = canonicalize(e.body, reserved);
    return make_rexpr<resolved_function>(e.name, e.args, body_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_argument& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_argument>(e);
}

r_expr canonicalize(const resolved_bind& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_bind>(e);
}

r_expr canonicalize(const resolved_initial& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    return make_rexpr<resolved_initial>(e.identifier, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_evolve& e, std::unordered_set<std::string>& reserved) {
    /// TODO: Should we be solving the ODE before we get to this step?
    auto val_canon = canonicalize(e.value, reserved);
    return make_rexpr<resolved_evolve>(e.identifier, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_effect& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    return make_rexpr<resolved_effect>(e.effect, e.ion, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_export& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_export>(e);
}

r_expr canonicalize(const resolved_call& e, std::unordered_set<std::string>& reserved) {
    std::vector<r_expr> args_canon;

    bool has_let = false;
    resolved_let let_outer;
    for (const auto& arg: e.call_args) {
        auto arg_canon = canonicalize(arg, reserved);
        if (auto let_first = std::get_if<resolved_let>(arg_canon.get())) {
            args_canon.push_back(get_innermost_body(let_first));
            if (!has_let) {
                let_outer = *let_first;
            } else {
                set_innermost_body(&let_outer, arg_canon);
            }
            has_let = true;
        }
        else {
            args_canon.push_back(arg_canon);
        }
    }
    auto call_canon = make_rexpr<resolved_call>(e.f_identifier, args_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_argument>(unique_local_name(reserved), e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, call_canon, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_object& e, std::unordered_set<std::string>& reserved) {
    std::vector<r_expr> values_canon;

    bool has_let = false;
    resolved_let let_outer;
    for (const auto& arg: e.record_values) {
        auto arg_canon = canonicalize(arg, reserved);
        if (auto let_first = std::get_if<resolved_let>(arg_canon.get())) {
            values_canon.push_back(get_innermost_body(let_first));
            if (!has_let) {
                let_outer = *let_first;
            }
            else {
                set_innermost_body(&let_outer, arg_canon);
            }
            has_let = true;
        }
        else {
            values_canon.push_back(arg_canon);
        }
    }
    auto object_canon = make_rexpr<resolved_object>(e.r_identifier, e.record_fields, values_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_argument>(unique_local_name(reserved), e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, object_canon, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_let& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    auto body_canon = canonicalize(e.body, reserved);
    auto let_outer = resolved_let(e.identifier, val_canon, body_canon, e.type, e.loc);
    if (auto let_first = std::get_if<resolved_let>(val_canon.get())) {
        let_outer.value = get_innermost_body(let_first);
        set_innermost_body(let_first, make_rexpr<resolved_let>(let_outer));
        return make_rexpr<resolved_let>(*let_first);
    }
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_conditional& e, std::unordered_set<std::string>& reserved) {
    auto cond_canon = canonicalize(e.condition, reserved);
    auto true_canon = canonicalize(e.value_true, reserved);
    auto false_canon = canonicalize(e.value_false, reserved);

    resolved_let let_outer;
    bool has_let = false;
    if (auto let_first = std::get_if<resolved_let>(cond_canon.get())) {
        let_outer = *let_first;
        cond_canon = get_innermost_body(let_first);
        has_let = true;
    }
    if (auto let_first = std::get_if<resolved_let>(true_canon.get())) {
        if (!has_let) {
            let_outer = *let_first;
        }
        else {
            set_innermost_body(&let_outer, true_canon);
        }
        true_canon = get_innermost_body(let_first);
    }
    if (auto let_first = std::get_if<resolved_let>(false_canon.get())) {
        if (!has_let) {
            let_outer = *let_first;
        }
        else {
            set_innermost_body(&let_outer, false_canon);
        }
        false_canon = get_innermost_body(let_first);
    }

    auto if_canon = make_rexpr<resolved_conditional>(cond_canon, true_canon, false_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_argument>(unique_local_name(reserved), e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, if_canon, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_float& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_float>(e);
}

r_expr canonicalize(const resolved_int& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_int>(e);
}

r_expr canonicalize(const resolved_unary& e, std::unordered_set<std::string>& reserved) {
    auto arg_canon = canonicalize(e.arg, reserved);

    resolved_let let_outer;
    bool has_let = false;
    if (auto let_first = std::get_if<resolved_let>(arg_canon.get())) {
        arg_canon = get_innermost_body(let_first);
        let_outer = *let_first;
        has_let = true;
    }
    auto unary_canon = make_rexpr<resolved_unary>(e.op, arg_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_argument>(unique_local_name(reserved), e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, unary_canon, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_binary& e, std::unordered_set<std::string>& reserved) {
    auto lhs_canon = canonicalize(e.lhs, reserved);
    auto rhs_canon = canonicalize(e.rhs, reserved);

    resolved_let let_outer;
    bool has_let = false;
    if (auto let_first = std::get_if<resolved_let>(lhs_canon.get())) {
        let_outer = *let_first;
        lhs_canon = get_innermost_body(let_first);
        has_let = true;
    }
    if (auto let_first = std::get_if<resolved_let>(rhs_canon.get())) {
        if (!has_let) {
            let_outer = *let_first;
        }
        else {
            set_innermost_body(&let_outer, rhs_canon);
        }
        rhs_canon = get_innermost_body(let_first);
        has_let = true;
    }

    auto binary_canon = make_rexpr<resolved_binary>(e.op, lhs_canon, rhs_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_argument>(unique_local_name(reserved), e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, binary_canon, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const r_expr& e, std::unordered_set<std::string>& reserved) {
    return std::visit([&](auto& c) {return canonicalize(c, reserved);}, *e);
}

r_expr canonicalize(const r_expr& e) {
    std::unordered_set<std::string> reserved;
    return canonicalize(e, reserved);
}

// SSA
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