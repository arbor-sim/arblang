#include <cmath>
#include <string>
#include <unordered_set>

#include <arblang/optimizer/copy_propagate.hpp>
#include <arblang/util/custom_hash.hpp>

namespace al {
namespace resolved_ir {
bool is_identifier(const r_expr& e) {
    return std::get_if<resolved_argument>(e.get());
}
bool is_object(const r_expr& e) {
    return std::get_if<resolved_object>(e.get());
}

std::pair<resolved_mechanism, bool> copy_propagate(const resolved_mechanism& e) {
    std::unordered_map<std::string, r_expr> local_copy_map;
    resolved_mechanism mech;
    bool made_changes = false;
    for (const auto& c: e.constants) {
        auto result = copy_propagate(c, local_copy_map);
        mech.constants.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.parameters) {
        local_copy_map.clear();
        auto result = copy_propagate(c, local_copy_map);
        mech.parameters.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.bindings) {
        local_copy_map.clear();
        auto result = copy_propagate(c, local_copy_map);
        mech.bindings.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.states) {
        local_copy_map.clear();
        auto result = copy_propagate(c, local_copy_map);
        mech.states.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.functions) {
        local_copy_map.clear();
        auto result = copy_propagate(c, local_copy_map);
        mech.functions.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.initializations) {
        local_copy_map.clear();
        auto result = copy_propagate(c, local_copy_map);
        mech.initializations.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.evolutions) {
        local_copy_map.clear();
        auto result = copy_propagate(c, local_copy_map);
        mech.evolutions.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.effects) {
        local_copy_map.clear();
        auto result = copy_propagate(c, local_copy_map);
        mech.effects.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.exports) {
        local_copy_map.clear();
        auto result = copy_propagate(c, local_copy_map);
        mech.exports.push_back(result.first);
        made_changes |= result.second;
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return {mech, made_changes};
}

std::pair<r_expr, bool> copy_propagate(const resolved_record_alias& e, std::unordered_map<std::string, r_expr>& copy_map) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation.");
}

std::pair<r_expr, bool> copy_propagate(const resolved_argument& e, std::unordered_map<std::string, r_expr>& copy_map) {
    if (copy_map.count(e.name)) return {copy_map.at(e.name), true};
    return {make_rexpr<resolved_argument>(e), false};
}

std::pair<r_expr, bool> copy_propagate(const resolved_variable& e, std::unordered_map<std::string, r_expr>& copy_map) {
    if (copy_map.count(e.name)) return {copy_map.at(e.name), true};
    return {make_rexpr<resolved_variable>(e), false};
}

std::pair<r_expr, bool> copy_propagate(const resolved_parameter& e, std::unordered_map<std::string, r_expr>& copy_map) {
    auto result = copy_propagate(e.value, copy_map);
    return {make_rexpr<resolved_parameter>(e.name, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> copy_propagate(const resolved_constant& e, std::unordered_map<std::string, r_expr>& copy_map) {
    auto result = copy_propagate(e.value, copy_map);
    return {make_rexpr<resolved_constant>(e.name, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> copy_propagate(const resolved_state& e, std::unordered_map<std::string, r_expr>& copy_map) {
    return {make_rexpr<resolved_state>(e), false};
}

std::pair<r_expr, bool> copy_propagate(const resolved_function& e, std::unordered_map<std::string, r_expr>& copy_map) {
    auto result = copy_propagate(e.body, copy_map);
    return {make_rexpr<resolved_function>(e.name, e.args, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> copy_propagate(const resolved_bind& e, std::unordered_map<std::string, r_expr>& copy_map) {
    return {make_rexpr<resolved_bind>(e), false};
}

std::pair<r_expr, bool> copy_propagate(const resolved_initial& e, std::unordered_map<std::string, r_expr>& copy_map) {
    auto result = copy_propagate(e.value, copy_map);
    return {make_rexpr<resolved_initial>(e.identifier, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> copy_propagate(const resolved_evolve& e, std::unordered_map<std::string, r_expr>& copy_map) {
    auto result = copy_propagate(e.value, copy_map);
    return {make_rexpr<resolved_evolve>(e.identifier, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> copy_propagate(const resolved_effect& e, std::unordered_map<std::string, r_expr>& copy_map) {
    auto result = copy_propagate(e.value, copy_map);
    return {make_rexpr<resolved_effect>(e.effect, e.ion, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> copy_propagate(const resolved_export& e, std::unordered_map<std::string, r_expr>& copy_map) {
    return {make_rexpr<resolved_export>(e), false};
}

std::pair<r_expr, bool> copy_propagate(const resolved_call& e, std::unordered_map<std::string, r_expr>& copy_map) {
    std::vector<r_expr> args;
    bool made_change = false;
    for (const auto& a: e.call_args) {
        auto result = copy_propagate(a, copy_map);
        args.push_back(result.first);
        made_change |= result.second;
    }
    return {make_rexpr<resolved_call>(e.f_identifier, args, e.type, e.loc), made_change};
}

std::pair<r_expr, bool> copy_propagate(const resolved_object& e, std::unordered_map<std::string, r_expr>& copy_map) {
    std::vector<r_expr> values;
    bool made_change = false;
    for (const auto& a: e.record_values) {
        auto result = copy_propagate(a, copy_map);
        values.push_back(result.first);
        made_change |= result.second;
    }
    return {make_rexpr<resolved_object>(e.record_fields, values, e.type, e.loc), made_change};
}

std::pair<r_expr, bool> copy_propagate(const resolved_let& e, std::unordered_map<std::string, r_expr>& copy_map) {
    if (is_identifier(e.value) || is_object(e.value)) {
        copy_map.insert({std::get<resolved_argument>(*e.identifier).name, e.value});
    }
    auto val  = copy_propagate(e.value, copy_map);
    auto body = copy_propagate(e.body, copy_map);
    return {make_rexpr<resolved_let>(e.identifier, val.first, body.first, e.type, e.loc), val.second||body.second};
}

std::pair<r_expr, bool> copy_propagate(const resolved_conditional& e, std::unordered_map<std::string, r_expr>& copy_map) {
    auto cond = copy_propagate(e.condition, copy_map);
    auto tval = copy_propagate(e.value_true, copy_map);
    auto fval = copy_propagate(e.value_false, copy_map);
    return {make_rexpr<resolved_conditional>(cond.first, tval.first, fval.first, e.type, e.loc),
            cond.second||tval.second||fval.second};
}

std::pair<r_expr, bool>  copy_propagate(const resolved_float& e, std::unordered_map<std::string, r_expr>& copy_map) {
    return {make_rexpr<resolved_float>(e), false};
}

std::pair<r_expr, bool> copy_propagate(const resolved_int& e, std::unordered_map<std::string, r_expr>& copy_map) {
    return {make_rexpr<resolved_int>(e), false};
}

std::pair<r_expr, bool> copy_propagate(const resolved_unary& e, std::unordered_map<std::string, r_expr>& copy_map) {
    auto arg = copy_propagate(e.arg, copy_map);
    return {make_rexpr<resolved_unary>(e.op, arg.first, e.type, e.loc), arg.second};
}

std::pair<r_expr, bool> copy_propagate(const resolved_binary& e, std::unordered_map<std::string, r_expr>& copy_map) {
    auto lhs_arg = copy_propagate(e.lhs, copy_map);
    auto rhs_arg = copy_propagate(e.rhs, copy_map);
    return {make_rexpr<resolved_binary>(e.op, lhs_arg.first, rhs_arg.first, e.type, e.loc), lhs_arg.second||rhs_arg.second};
}

std::pair<r_expr, bool> copy_propagate(const resolved_field_access& e, std::unordered_map<std::string, r_expr>& copy_map) {
    auto obj_arg = copy_propagate(e.object, copy_map);
    return {make_rexpr<resolved_field_access>(obj_arg.first, e.field, e.type, e.loc), obj_arg.second};
}

std::pair<r_expr, bool> copy_propagate(const r_expr& e, std::unordered_map<std::string, r_expr>& copy_map) {
    return std::visit([&](auto& c) {return copy_propagate(c, copy_map);}, *e);
}

std::pair<r_expr, bool> copy_propagate(const r_expr& e) {
    std::unordered_map<std::string, r_expr> copy_map;
    return copy_propagate(e, copy_map);
}

} // namespace resolved_ir
} // namespace al