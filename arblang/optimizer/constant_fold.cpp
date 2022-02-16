#include <cmath>
#include <string>
#include <unordered_set>

#include <arblang/optimizer/constant_fold.hpp>
#include <arblang/util/custom_hash.hpp>

namespace al {
namespace resolved_ir {

std::optional<double> as_number(const r_expr& e) {
    if (auto v = std::get_if<resolved_float>(e.get())) {
        return v->value;
    }
    if (auto v = std::get_if<resolved_int>(e.get())) {
        return v->value;
    }
    return {};
}

bool is_integer(double v) {
    return std::floor(v) == v;
}

resolved_mechanism constant_fold(const resolved_mechanism& e) {
    std::unordered_map<std::string, r_expr> constants_map, local_constant_map;
    resolved_mechanism mech;
    for (const auto& c: e.constants) {
        auto name = std::get<resolved_constant>(*c).name;
        auto cst = constant_fold(c, local_constant_map);
        if (as_number(cst)) constants_map.insert({name, cst});
        mech.constants.push_back(cst);
    }
    for (const auto& c: e.parameters) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        mech.parameters.push_back(constant_fold(c, local_constant_map));
    }
    for (const auto& c: e.bindings) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        mech.bindings.push_back(constant_fold(c, local_constant_map));
    }
    for (const auto& c: e.states) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        mech.states.push_back(constant_fold(c, local_constant_map));
    }
    for (const auto& c: e.functions) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        mech.functions.push_back(constant_fold(c, local_constant_map));
    }
    for (const auto& c: e.initializations) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        mech.initializations.push_back(constant_fold(c, local_constant_map));
    }
    for (const auto& c: e.evolutions) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        mech.evolutions.push_back(constant_fold(c, local_constant_map));
    }
    for (const auto& c: e.effects) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        mech.effects.push_back(constant_fold(c, local_constant_map));
    }
    for (const auto& c: e.exports) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        mech.exports.push_back(constant_fold(c, local_constant_map));
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return mech;
}

r_expr constant_fold(const resolved_parameter& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_parameter>(e.name, constant_fold(e.value, constant_map), e.type, e.loc);
}

r_expr constant_fold(const resolved_constant& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_constant>(e.name, constant_fold(e.value, constant_map), e.type, e.loc);
}

r_expr constant_fold(const resolved_state& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_state>(e);
}

r_expr constant_fold(const resolved_record_alias& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_record_alias>(e);
}

r_expr constant_fold(const resolved_function& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_function>(e.name, e.args, constant_fold(e.body, constant_map), e.type, e.loc);
}

r_expr constant_fold(const resolved_argument& e, std::unordered_map<std::string, r_expr>& constant_map) {
    if (constant_map.count(e.name)) return constant_map.at(e.name);
    return make_rexpr<resolved_argument>(e);
}

r_expr constant_fold(const resolved_bind& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_bind>(e);
}

r_expr constant_fold(const resolved_initial& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_initial>(e.identifier, constant_fold(e.value, constant_map), e.type, e.loc);
}

r_expr constant_fold(const resolved_evolve& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_evolve>(e.identifier, constant_fold(e.value, constant_map), e.type, e.loc);
}

r_expr constant_fold(const resolved_effect& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_effect>(e.effect, e.ion, constant_fold(e.value, constant_map), e.type, e.loc);
}

r_expr constant_fold(const resolved_export& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_export>(e);
}

r_expr constant_fold(const resolved_call& e, std::unordered_map<std::string, r_expr>& constant_map) {
    std::vector<r_expr> args;
    for (const auto& a: e.call_args) {
        args.push_back(constant_fold(a, constant_map));
    }
    return make_rexpr<resolved_call>(e.f_identifier, args, e.type, e.loc);
}

r_expr constant_fold(const resolved_object& e, std::unordered_map<std::string, r_expr>& constant_map) {
    std::vector<r_expr> values;
    for (const auto& a: e.record_values) {
        values.push_back(constant_fold(a, constant_map));
    }
    return make_rexpr<resolved_object>(e.r_identifier, e.record_fields, values, e.type, e.loc);
}

r_expr constant_fold(const resolved_let& e, std::unordered_map<std::string, r_expr>& constant_map) {
    if (as_number(e.value)) {
        constant_map.insert({std::get<resolved_argument>(*e.identifier).name, e.value});
    }
    auto val_cst  = constant_fold(e.value, constant_map);
    auto body_cst = constant_fold(e.body, constant_map);
    return make_rexpr<resolved_let>(e.identifier, val_cst, body_cst, e.type, e.loc);
}

r_expr constant_fold(const resolved_conditional& e,std::unordered_map<std::string, r_expr>& constant_map) {
    auto cond = constant_fold(e.condition, constant_map);
    auto tval = constant_fold(e.value_true, constant_map);
    auto fval = constant_fold(e.value_false, constant_map);

    if (auto val = as_number(cond)) {
        if (val.value()) {
            return tval;
        }
        return fval;
    }
    return make_rexpr<resolved_conditional>(e.condition, tval, fval, e.type, e.loc);
}

r_expr constant_fold(const resolved_float& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_float>(e);
}

r_expr constant_fold(const resolved_int& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return make_rexpr<resolved_int>(e);
}

r_expr constant_fold(const resolved_unary& e, std::unordered_map<std::string, r_expr>& constant_map) {
    if (auto val_opt = as_number(constant_fold(e.arg, constant_map))) {
        auto val = val_opt.value();
        switch (e.op) {
            case unary_op::exp:
                val = std::exp(val); break;
            case unary_op::log:
                val = std::log(val); break;
            case unary_op::cos:
                val = std::cos(val); break;
            case unary_op::sin:
                val = std::sin(val); break;
            case unary_op::abs:
                val = std::abs(val); break;
            case unary_op::exprelr:
                val = val/(std::log(val)-1); break;
            case unary_op::lnot:
                val = !((bool)val); break;
            case unary_op::neg:
                val = -val; break;
        }
        if (is_integer(val)) {
            return make_rexpr<resolved_int>(val, e.type, e.loc);
        }
        return make_rexpr<resolved_float>(val, e.type, e.loc);
    }
    return make_rexpr<resolved_unary>(e);
}

r_expr constant_fold(const resolved_binary& e, std::unordered_map<std::string, r_expr>& constant_map) {
    auto lhs_opt = as_number(constant_fold(e.lhs, constant_map));
    auto rhs_opt = as_number(constant_fold(e.rhs, constant_map));
    if (lhs_opt && rhs_opt) {
        auto lhs = lhs_opt.value();
        auto rhs = rhs_opt.value();
        double val;
        switch (e.op) {
            case binary_op::add:
                val = lhs + rhs; break;
            case binary_op::sub:
                val = lhs - rhs; break;
            case binary_op::mul:
                val = lhs * rhs; break;
            case binary_op::div:
                val = lhs / rhs; break;
            case binary_op::pow:
                val = std::pow(lhs, rhs); break;
            case binary_op::lt:
                val = lhs < rhs; break;
            case binary_op::le:
                val = lhs <= rhs; break;
            case binary_op::gt:
                val = lhs > rhs; break;
            case binary_op::ge:
                val = lhs >= rhs; break;
            case binary_op::eq:
                val = lhs == rhs; break;
            case binary_op::ne:
                val = lhs != rhs; break;
            case binary_op::land:
                val = lhs && rhs; break;
            case binary_op::lor:
                val = lhs || rhs; break;
            case binary_op::min:
                val = std::min(lhs, rhs); break;
            case binary_op::max:
                val = std::max(lhs, rhs); break;
            case binary_op::dot:
                break;
        }
        if (is_integer(val)) {
            return make_rexpr<resolved_int>(val, e.type, e.loc);
        }
        return make_rexpr<resolved_float>(val, e.type, e.loc);
    }
    return make_rexpr<resolved_binary>(e);
}

r_expr constant_fold(const r_expr& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return std::visit([&](auto& c) {return constant_fold(c, constant_map);}, *e);
}

r_expr constant_fold(const r_expr& e) {
    std::unordered_map<std::string, r_expr> constant_map;
    return constant_fold(e, constant_map);
}

} // namespace resolved_ir
} // namespace al