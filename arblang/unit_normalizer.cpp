#include <optional>
#include <stdexcept>
#include <string>

#define FMT_HEADER_ONLY YES
#include <fmt/core.h>
#include <fmt/format.h>

#include <arblang/unit_normalizer.hpp>
#include <arblang/unit_expressions.hpp>
#include <arblang/raw_expressions.hpp>

namespace al {

unit_normalizer::unit_normalizer(expr e): expression(std::move(e)) {}

expr unit_normalizer::normalize() {
    return std::visit([&](auto&& c){return normalize(c);}, *expression);
}

expr unit_normalizer::normalize(const mechanism_expr& e) {
    mechanism_expr mech;
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
    return make_expr<mechanism_expr>(mech);
}
expr unit_normalizer::normalize(const parameter_expr& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_expr<parameter_expr>(e.identifier, val, e.loc);
}
expr unit_normalizer::normalize(const constant_expr& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_expr<constant_expr>(e.identifier, val, e.loc);
}
expr unit_normalizer::normalize(const state_expr& e) {
    return make_expr<state_expr>(e);
}
expr unit_normalizer::normalize(const record_alias_expr& e) {
    return make_expr<record_alias_expr>(e);
}
expr unit_normalizer::normalize(const function_expr& e) {
    auto body = std::visit([&](auto&& c){return normalize(c);}, *e.body);
    return make_expr<function_expr>(e.name, e.args, e.ret, body, e.loc);
}
expr unit_normalizer::normalize(const bind_expr& e) {
    return make_expr<bind_expr>(e);
}
expr unit_normalizer::normalize(const initial_expr& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_expr<initial_expr>(e.identifier, val, e.loc);
}
expr unit_normalizer::normalize(const evolve_expr& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_expr<evolve_expr>(e.identifier, val, e.loc);
}
expr unit_normalizer::normalize(const effect_expr& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_expr<effect_expr>(e.effect, e.ion, e.type, val, e.loc);
}
expr unit_normalizer::normalize(const export_expr& e) {
    return make_expr<export_expr>(e);
}
expr unit_normalizer::normalize(const call_expr& e) {
    std::vector<expr> args;
    for (const auto& a:e.call_args) {
        args.push_back(std::visit([&](auto&& c){return normalize(c);}, *a));
    }
    return make_expr<call_expr>(e.function_name, args, e.loc);
}
expr unit_normalizer::normalize(const object_expr& e) {
    std::vector<expr> record_values;
    for (const auto& a:e.record_values) {
        record_values.push_back(std::visit([&](auto&& c){return normalize(c);}, *a));
    }
    return make_expr<object_expr>(e.record_name, e.record_fields, record_values, e.loc);
}
expr unit_normalizer::normalize(const let_expr& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    auto body = std::visit([&](auto&& c){return normalize(c);}, *e.body);
    return make_expr<let_expr>(e.identifier, val, body, e.loc);
}
expr unit_normalizer::normalize(const with_expr& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    auto body = std::visit([&](auto&& c){return normalize(c);}, *e.body);
    return make_expr<with_expr>(val, body, e.loc);
}
expr unit_normalizer::normalize(const conditional_expr& e) {
    auto cond = std::visit([&](auto&& c){return normalize(c);}, *e.condition);
    auto val_true  = std::visit([&](auto&& c){return normalize(c);}, *e.value_true);
    auto val_false = std::visit([&](auto&& c){return normalize(c);}, *e.value_true);
    return make_expr<conditional_expr>(cond, val_true, val_false, e.loc);
}
expr unit_normalizer::normalize(const identifier_expr& e) {
    return make_expr<identifier_expr>(e);
}
expr unit_normalizer::normalize(const unary_expr& e) {
    auto val = std::visit([&](auto&& c){return normalize(c);}, *e.value);
    return make_expr<unary_expr>(e.op, val, e.loc);
}
expr unit_normalizer::normalize(const binary_expr& e) {
    auto lhs = std::visit([&](auto&& c){return normalize(c);}, *e.lhs);
    auto rhs = std::visit([&](auto&& c){return normalize(c);}, *e.rhs);
    return make_expr<binary_expr>(e.op, lhs, rhs, e.loc);
}
expr unit_normalizer::normalize(const float_expr& e) {
    auto unit = std::visit([&](auto&& c){return u_raw_ir::normalize_unit(c);}, *e.unit);
    double val = e.value*std::pow(10,unit.second);
    if (val == (int)val) {
        return make_expr<int_expr>(val, unit.first, e.loc);
    }
    return make_expr<float_expr>(val, unit.first, e.loc);
}
expr unit_normalizer::normalize(const int_expr& e) {
    auto unit = std::visit([&](auto&& c){return u_raw_ir::normalize_unit(c);}, *e.unit);
    double val = (double)e.value*std::pow(10,unit.second);
    if (val == (int)val) {
        return make_expr<int_expr>(val, unit.first, e.loc);
    }
    return make_expr<float_expr>(val, unit.first, e.loc);
}
} // namespace al