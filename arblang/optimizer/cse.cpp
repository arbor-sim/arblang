#include <string>
#include <unordered_set>

#include <arblang/optimizer/cse.hpp>
#include <arblang/util/custom_hash.hpp>

namespace al {
namespace resolved_ir {

// TODO make sure that single_assign is called before cse
std::pair<resolved_mechanism, bool> cse(const resolved_mechanism& e) {
    std::unordered_map<resolved_expr, r_expr> expr_map;
    resolved_mechanism mech;
    bool made_changes = false;
    for (const auto& c: e.constants) {
        expr_map.clear();
        auto result = cse(c, expr_map);
        mech.constants.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.parameters) {
        expr_map.clear();
        auto result = cse(c, expr_map);
        mech.parameters.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.bindings) {
        expr_map.clear();
        auto result = cse(c, expr_map);
        mech.bindings.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.states) {
        expr_map.clear();
        auto result = cse(c, expr_map);
        mech.states.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.functions) {
        expr_map.clear();
        auto result = cse(c, expr_map);
        mech.functions.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.initializations) {
        expr_map.clear();
        auto result = cse(c, expr_map);
        mech.initializations.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.evolutions) {
        expr_map.clear();
        auto result = cse(c, expr_map);
        mech.evolutions.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.effects) {
        expr_map.clear();
        auto result = cse(c, expr_map);
        mech.effects.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.exports) {
        expr_map.clear();
        auto result = cse(c, expr_map);
        mech.exports.push_back(result.first);
        made_changes |= result.second;
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return {mech, made_changes};
}

std::pair<r_expr, bool> cse(const resolved_record_alias& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation.");
}

std::pair<r_expr, bool> cse(const resolved_argument& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_argument>(e), false};
}

std::pair<r_expr, bool> cse(const resolved_variable& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_variable>(e), false};
}

std::pair<r_expr, bool> cse(const resolved_parameter& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    auto val_cse = cse(e.value, expr_map);
    return {make_rexpr<resolved_parameter>(e.name, val_cse.first, e.type, e.loc), val_cse.second};
}

std::pair<r_expr, bool> cse(const resolved_constant& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    auto val_cse = cse(e.value, expr_map);
    return {make_rexpr<resolved_constant>(e.name, val_cse.first, e.type, e.loc), val_cse.second};
}

std::pair<r_expr, bool> cse(const resolved_state& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_state>(e), false};
}

std::pair<r_expr, bool> cse(const resolved_function& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    auto body_cse = cse(e.body, expr_map);
    return {make_rexpr<resolved_function>(e.name, e.args, body_cse.first, e.type, e.loc), body_cse.second};
}

std::pair<r_expr, bool> cse(const resolved_bind& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_bind>(e), false};
}

std::pair<r_expr, bool> cse(const resolved_initial& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    auto val_cse = cse(e.value, expr_map);
    return {make_rexpr<resolved_initial>(e.identifier, val_cse.first, e.type, e.loc), val_cse.second};
}

std::pair<r_expr, bool> cse(const resolved_evolve& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    auto val_cse = cse(e.value, expr_map);
    return {make_rexpr<resolved_evolve>(e.identifier, val_cse.first, e.type, e.loc), val_cse.second};
}

std::pair<r_expr, bool> cse(const resolved_effect& e, std::unordered_map<resolved_expr, r_expr>& expr_map){
    auto val_cse = cse(e.value, expr_map);
    return {make_rexpr<resolved_effect>(e.effect, e.ion, val_cse.first, e.type, e.loc), val_cse.second};
}

std::pair<r_expr, bool> cse(const resolved_export& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_export>(e), false};
}

// TODO: Do we need to visit the args?
std::pair<r_expr, bool> cse(const resolved_call& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_call>(e), false};
}

// TODO: Do we need to visit the args?
std::pair<r_expr, bool> cse(const resolved_object& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_object>(e), false};
}

std::pair<r_expr, bool> cse(const resolved_let& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    bool made_change = false;
    auto val = e.value;
    if (!expr_map.insert({*e.value, e.identifier}).second) {
        val = expr_map[*e.value];
        made_change = true;
    }
    auto body_cse = cse(e.body, expr_map);
    return {make_rexpr<resolved_let>(e.identifier, val, body_cse.first, e.type, e.loc), made_change||body_cse.second};
}

// TODO: Do we need to visit the args?
std::pair<r_expr, bool> cse(const resolved_conditional& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_conditional>(e), false};
}

std::pair<r_expr, bool> cse(const resolved_float& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_float>(e), false};
}

std::pair<r_expr, bool> cse(const resolved_int& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_int>(e), false};
}

std::pair<r_expr, bool> cse(const resolved_unary& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_unary>(e), false};
}

std::pair<r_expr, bool> cse(const resolved_binary& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_binary>(e), false};
}

std::pair<r_expr, bool> cse(const resolved_field_access& e, std::unordered_map<resolved_expr, r_expr>& expr_map) {
    return {make_rexpr<resolved_field_access>(e), false};
}

std::pair<r_expr, bool> cse(const r_expr& e, std::unordered_map<resolved_expr, r_expr>& expr_map){
    return std::visit([&](auto& c) {return cse(c, expr_map);}, *e);
}

std::pair<r_expr, bool> cse(const r_expr& e) {
    std::unordered_map<resolved_expr, r_expr> expr_map;
    return cse(e, expr_map);
}

} // namespace resolved_ir
} // namespace al