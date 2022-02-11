#include <fmt/format.h>

#include <arblang/parser/normalizer.hpp>
#include <arblang/parser/unit_expressions.hpp>
#include <arblang/parser/parsed_expressions.hpp>

namespace al {
namespace parsed_ir {

p_expr normalize(p_expr e) {
    return std::visit([&](auto&& c){return normalize(c);}, *e);
}

parsed_mechanism normalize(const parsed_mechanism& e) {
    parsed_mechanism mech;
    mech.name = e.name;
    mech.loc  = e.loc;
    mech.kind = e.kind;
    for (const auto& c: e.constants) {
        mech.constants.push_back(std::visit([&](auto&& c){return normalize(c);}, *c));
    }
    for (const auto& c: e.parameters) {
        mech.parameters.push_back(std::visit([&](auto&& c){return normalize(c);}, *c));
    }
    for (const auto& c: e.states) {
        mech.states.push_back(std::visit([&](auto&& c){return normalize(c);}, *c));
    }
    for (const auto& c: e.functions) {
        mech.functions.push_back(std::visit([&](auto&& c){return normalize(c);}, *c));
    }
    for (const auto& c: e.records) {
        mech.records.push_back(std::visit([&](auto&& c){return normalize(c);}, *c));
    }
    for (const auto& c: e.bindings) {
        mech.bindings.push_back(std::visit([&](auto&& c){return normalize(c);}, *c));
    }
    for (const auto& c: e.initializations) {
        mech.initializations.push_back(std::visit([&](auto&& c){return normalize(c);}, *c));
    }
    for (const auto& c: e.effects) {
        mech.effects.push_back(std::visit([&](auto&& c){return normalize(c);}, *c));
    }
    for (const auto& c: e.evolutions) {
        mech.evolutions.push_back(std::visit([&](auto&& c){return normalize(c);}, *c));
    }
    for (const auto& c: e.exports) {
        mech.exports.push_back(std::visit([&](auto&& c){return normalize(c);}, *c));
    }
    return mech;
}
p_expr normalize(const parsed_parameter& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_pexpr<parsed_parameter>(e.identifier, val, e.loc);
}
p_expr normalize(const parsed_constant& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_pexpr<parsed_constant>(e.identifier, val, e.loc);
}
p_expr normalize(const parsed_state& e) {
    return make_pexpr<parsed_state>(e);
}
p_expr normalize(const parsed_record_alias& e) {
    return make_pexpr<parsed_record_alias>(e);
}
p_expr normalize(const parsed_function& e) {
    auto body = std::visit([&](auto&& c){return normalize(c);}, *e.body);
    return make_pexpr<parsed_function>(e.name, e.args, e.ret, body, e.loc);
}
p_expr normalize(const parsed_bind& e) {
    return make_pexpr<parsed_bind>(e);
}
p_expr normalize(const parsed_initial& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_pexpr<parsed_initial>(e.identifier, val, e.loc);
}
p_expr normalize(const parsed_evolve& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_pexpr<parsed_evolve>(e.identifier, val, e.loc);
}
p_expr normalize(const parsed_effect& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_pexpr<parsed_effect>(e.effect, e.ion, e.type, val, e.loc);
}
p_expr normalize(const parsed_export& e) {
    return make_pexpr<parsed_export>(e);
}
p_expr normalize(const parsed_call& e) {
    std::vector<p_expr> args;
    for (const auto& a:e.call_args) {
        args.push_back(std::visit([&](auto&& c){return normalize(c);}, *a));
    }
    return make_pexpr<parsed_call>(e.function_name, args, e.loc);
}
p_expr normalize(const parsed_object& e) {
    std::vector<p_expr> record_values;
    for (const auto& a:e.record_values) {
        record_values.push_back(std::visit([&](auto&& c){return normalize(c);}, *a));
    }
    return make_pexpr<parsed_object>(e.record_name, e.record_fields, record_values, e.loc);
}
p_expr normalize(const parsed_let& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    auto body = std::visit([&](auto&& c){return normalize(c);}, *e.body);
    return make_pexpr<parsed_let>(e.identifier, val, body, e.loc);
}
p_expr normalize(const parsed_with& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    auto body = std::visit([&](auto&& c){return normalize(c);}, *e.body);
    return make_pexpr<parsed_with>(val, body, e.loc);
}
p_expr normalize(const parsed_conditional& e) {
    auto cond = std::visit([&](auto&& c){return normalize(c);}, *e.condition);
    auto val_true  = std::visit([&](auto&& c){return normalize(c);}, *e.value_true);
    auto val_false = std::visit([&](auto&& c){return normalize(c);}, *e.value_false);
    return make_pexpr<parsed_conditional>(cond, val_true, val_false, e.loc);
}
p_expr normalize(const parsed_identifier& e) {
    return make_pexpr<parsed_identifier>(e);
}
p_expr normalize(const parsed_unary& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_pexpr<parsed_unary>(e.op, val, e.loc);
}
p_expr normalize(const parsed_binary& e) {
    auto lhs = std::visit([&](auto&& c){return normalize(c);}, *e.lhs);
    auto rhs = std::visit([&](auto&& c){return normalize(c);}, *e.rhs);
    return make_pexpr<parsed_binary>(e.op, lhs, rhs, e.loc);
}
p_expr normalize(const parsed_float& e) {
    auto unit = parsed_unit_ir::normalize_unit(e.unit);
    double val = e.value*std::pow(10,unit.second);
    if (val == (int)val) {
        return make_pexpr<parsed_int>(val, unit.first, e.loc);
    }
    return make_pexpr<parsed_float>(val, unit.first, e.loc);
}
p_expr normalize(const parsed_int& e) {
    auto unit = parsed_unit_ir::normalize_unit(e.unit);
    double val = (double)e.value*std::pow(10,unit.second);
    if (val == (int)val) {
        return make_pexpr<parsed_int>(val, unit.first, e.loc);
    }
    return make_pexpr<parsed_float>(val, unit.first, e.loc);
}
} // namespace parsed_ir
} // namespace al