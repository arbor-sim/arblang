#include <fmt/format.h>

#include <arblang/parser/normalizer.hpp>
#include <arblang/parser/parsed_units.hpp>
#include <arblang/parser/parsed_expressions.hpp>

namespace al {
namespace parsed_ir {

parsed_mechanism normalize(const parsed_mechanism& e) {
    parsed_mechanism mech;
    mech.name = e.name;
    mech.loc  = e.loc;
    mech.kind = e.kind;
    for (const auto& c: e.constants) {
        mech.constants.push_back(normalize(c));
    }
    for (const auto& c: e.parameters) {
        mech.parameters.push_back(normalize(c));
    }
    for (const auto& c: e.states) {
        mech.states.push_back(normalize(c));
    }
    for (const auto& c: e.functions) {
        mech.functions.push_back(normalize(c));
    }
    for (const auto& c: e.records) {
        mech.records.push_back(normalize(c));
    }
    for (const auto& c: e.bindings) {
        mech.bindings.push_back(normalize(c));
    }
    for (const auto& c: e.initializations) {
        mech.initializations.push_back(normalize(c));
    }
    for (const auto& c: e.on_events) {
        mech.on_events.push_back(normalize(c));
    }
    for (const auto& c: e.effects) {
        mech.effects.push_back(normalize(c));
    }
    for (const auto& c: e.evolutions) {
        mech.evolutions.push_back(normalize(c));
    }
    for (const auto& c: e.exports) {
        mech.exports.push_back(normalize(c));
    }
    return mech;
}
p_expr normalize(const parsed_parameter& e) {
    return make_pexpr<parsed_parameter>(e.identifier, normalize(e.value), e.loc);
}
p_expr normalize(const parsed_constant& e) {
    return make_pexpr<parsed_constant>(e.identifier, normalize(e.value), e.loc);
}
p_expr normalize(const parsed_state& e) {
    return make_pexpr<parsed_state>(e);
}
p_expr normalize(const parsed_record_alias& e) {
    return make_pexpr<parsed_record_alias>(e);
}
p_expr normalize(const parsed_function& e) {
    return make_pexpr<parsed_function>(e.name, e.args, e.ret, normalize(e.body), e.loc);
}
p_expr normalize(const parsed_bind& e) {
    return make_pexpr<parsed_bind>(e);
}
p_expr normalize(const parsed_initial& e) {
    return make_pexpr<parsed_initial>(e.identifier, normalize(e.value), e.loc);
}
p_expr normalize(const parsed_on_event& e) {
    return make_pexpr<parsed_on_event>(e.argument, e.identifier, normalize(e.value), e.loc);
}
p_expr normalize(const parsed_evolve& e) {
    return make_pexpr<parsed_evolve>(e.identifier, normalize(e.value), e.loc);
}
p_expr normalize(const parsed_effect& e) {
    return make_pexpr<parsed_effect>(e.effect, e.ion, e.type, normalize(e.value), e.loc);
}
p_expr normalize(const parsed_export& e) {
    return make_pexpr<parsed_export>(e);
}
p_expr normalize(const parsed_call& e) {
    std::vector<p_expr> args;
    for (const auto& a:e.call_args) {
        args.push_back(normalize(a));
    }
    return make_pexpr<parsed_call>(e.function_name, args, e.loc);
}
p_expr normalize(const parsed_object& e) {
    std::vector<p_expr> record_values;
    for (const auto& a:e.record_values) {
        record_values.push_back(normalize(a));
    }
    return make_pexpr<parsed_object>(e.record_name, e.record_fields, record_values, e.loc);
}
p_expr normalize(const parsed_let& e) {
    return make_pexpr<parsed_let>(e.identifier, normalize(e.value), normalize(e.body), e.loc);
}
p_expr normalize(const parsed_with& e) {
    return make_pexpr<parsed_with>(normalize(e.value), normalize(e.body), e.loc);
}
p_expr normalize(const parsed_conditional& e) {
    return make_pexpr<parsed_conditional>(normalize(e.condition), normalize(e.value_true), normalize(e.value_false), e.loc);
}
p_expr normalize(const parsed_identifier& e) {
    return make_pexpr<parsed_identifier>(e);
}
p_expr normalize(const parsed_unary& e) {
    return make_pexpr<parsed_unary>(e.op, normalize(e.value), e.loc);
}
p_expr normalize(const parsed_binary& e) {
    return make_pexpr<parsed_binary>(e.op, normalize(e.lhs), normalize(e.rhs), e.loc);
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

p_expr normalize(const p_expr& e) {
    return std::visit([&](auto&& c){return normalize(c);}, *e);
}
} // namespace parsed_ir
} // namespace al