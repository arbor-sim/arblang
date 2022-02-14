#include <string>
#include <unordered_set>

#include <arblang/optimizer/cse.hpp>
#include <arblang/util/custom_hash.hpp>

namespace al {
namespace resolved_ir {

// TODO make sure that single_assign is called before cse
resolved_mechanism cse(const resolved_mechanism& e) {
    std::unordered_map<resolved_expr, r_expr> expr_map;
    resolved_mechanism mech;
    for (const auto& c: e.constants) {
        expr_map.clear();
        mech.constants.push_back(cse(c, expr_map));
    }
    for (const auto& c: e.parameters) {
        expr_map.clear();
        mech.parameters.push_back(cse(c, expr_map));
    }
    for (const auto& c: e.bindings) {
        expr_map.clear();
        mech.bindings.push_back(cse(c, expr_map));
    }
    for (const auto& c: e.states) {
        expr_map.clear();
        mech.states.push_back(cse(c, expr_map));
    }
    for (const auto& c: e.functions) {
        expr_map.clear();
        mech.functions.push_back(cse(c, expr_map));
    }
    for (const auto& c: e.initializations) {
        expr_map.clear();
        mech.initializations.push_back(cse(c, expr_map));
    }
    for (const auto& c: e.evolutions) {
        expr_map.clear();
        mech.evolutions.push_back(cse(c, expr_map));
    }
    for (const auto& c: e.effects) {
        expr_map.clear();
        mech.effects.push_back(cse(c, expr_map));
    }
    for (const auto& c: e.exports) {
        expr_map.clear();
        mech.exports.push_back(cse(c,expr_map));
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return mech;
}

r_expr cse(const resolved_parameter& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_parameter>(e.name, cse(e.value, expr_map), e.type, e.loc);
}

r_expr cse(const resolved_constant& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_constant>(e.name, cse(e.value, expr_map), e.type, e.loc);
}

r_expr cse(const resolved_state& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_state>(e);
}

r_expr cse(const resolved_record_alias& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_record_alias>(e);
}

r_expr cse(const resolved_function& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_function>(e.name, e.args, cse(e.body, expr_map), e.type, e.loc);
}

r_expr cse(const resolved_argument& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_argument>(e);
}

r_expr cse(const resolved_bind& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_bind>(e);
}

r_expr cse(const resolved_initial& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_initial>(e.identifier, cse(e.value, expr_map), e.type, e.loc);
}

r_expr cse(const resolved_evolve& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_evolve>(e.identifier, cse(e.value, expr_map), e.type, e.loc);
}

r_expr cse(const resolved_effect& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_effect>(e.effect, e.ion, cse(e.value, expr_map), e.type, e.loc);
}

r_expr cse(const resolved_export& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_export>(e);
}

// TODO: Do we need to visit the args?
r_expr cse(const resolved_call& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_call>(e);
}

// TODO: Do we need to visit the args?
r_expr cse(const resolved_object& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_object>(e);
}

r_expr cse(const resolved_let& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    auto val_cse = e.value;
    if (!expr_map.insert({*e.value, e.identifier}).second) {
        val_cse = expr_map[*e.value];
    }
    auto body_cse = cse(e.body, expr_map);
    return make_rexpr<resolved_let>(e.identifier, val_cse, body_cse, e.type, e.loc);
}

// TODO: Do we need to visit the args?
r_expr cse(const resolved_conditional& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_conditional>(e);
}

r_expr cse(const resolved_float& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_float>(e);
}

r_expr cse(const resolved_int& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_int>(e);
}

r_expr cse(const resolved_unary& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_unary>(e);
}

// TODO: Do we need to visit the args?
r_expr cse(const resolved_binary& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return make_rexpr<resolved_binary>(e);
}

r_expr cse(const r_expr& e,
           std::unordered_map<resolved_expr, r_expr>& expr_map)
{
    return std::visit([&](auto& c) {return cse(c, expr_map);}, *e);
}

r_expr cse(const r_expr& e) {
    std::unordered_map<resolved_expr, r_expr> expr_map;
    return cse(e, expr_map);
}

} // namespace resolved_ir
} // namespace al