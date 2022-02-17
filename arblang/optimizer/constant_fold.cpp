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

std::pair<resolved_mechanism, bool> constant_fold(const resolved_mechanism& e) {
    std::unordered_map<std::string, r_expr> constants_map, local_constant_map;
    resolved_mechanism mech;
    bool made_changes = false;
    for (const auto& c: e.constants) {
        auto name = std::get<resolved_constant>(*c).name;
        auto result = constant_fold(c, local_constant_map);
        if (as_number(result.first)) constants_map.insert({name, result.first});
        mech.constants.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.parameters) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        auto result = constant_fold(c, local_constant_map);
        mech.parameters.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.bindings) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        auto result = constant_fold(c, local_constant_map);
        mech.bindings.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.states) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        auto result = constant_fold(c, local_constant_map);
        mech.states.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.functions) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        auto result = constant_fold(c, local_constant_map);
        mech.functions.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.initializations) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        auto result = constant_fold(c, local_constant_map);
        mech.initializations.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.evolutions) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        auto result = constant_fold(c, local_constant_map);
        mech.evolutions.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.effects) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        auto result = constant_fold(c, local_constant_map);
        mech.effects.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.exports) {
        local_constant_map.clear();
        local_constant_map = constants_map;
        auto result = constant_fold(c, local_constant_map);
        mech.exports.push_back(result.first);
        made_changes |= result.second;
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return {mech, made_changes};
}

std::pair<r_expr, bool> constant_fold(const resolved_parameter& e, std::unordered_map<std::string, r_expr>& constant_map) {
    auto result = constant_fold(e.value, constant_map);
    return {make_rexpr<resolved_parameter>(e.name, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_constant& e, std::unordered_map<std::string, r_expr>& constant_map) {
    auto result = constant_fold(e.value, constant_map);
    return {make_rexpr<resolved_constant>(e.name, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_state& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return {make_rexpr<resolved_state>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_record_alias& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return {make_rexpr<resolved_record_alias>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_function& e, std::unordered_map<std::string, r_expr>& constant_map) {
    auto result = constant_fold(e.body, constant_map);
    return {make_rexpr<resolved_function>(e.name, e.args, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_argument& e, std::unordered_map<std::string, r_expr>& constant_map) {
    if (constant_map.count(e.name)) return {constant_map.at(e.name), true};
    return {make_rexpr<resolved_argument>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_bind& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return {make_rexpr<resolved_bind>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_initial& e, std::unordered_map<std::string, r_expr>& constant_map) {
    auto result = constant_fold(e.value, constant_map);
    return {make_rexpr<resolved_initial>(e.identifier, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_evolve& e, std::unordered_map<std::string, r_expr>& constant_map) {
    auto result = constant_fold(e.value, constant_map);
    return {make_rexpr<resolved_evolve>(e.identifier, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_effect& e, std::unordered_map<std::string, r_expr>& constant_map) {
    auto result = constant_fold(e.value, constant_map);
    return {make_rexpr<resolved_effect>(e.effect, e.ion, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_export& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return {make_rexpr<resolved_export>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_call& e, std::unordered_map<std::string, r_expr>& constant_map) {
    std::vector<r_expr> args;
    bool made_change = false;
    for (const auto& a: e.call_args) {
        auto result = constant_fold(a, constant_map);
        args.push_back(result.first);
        made_change |= result.second;
    }
    return {make_rexpr<resolved_call>(e.f_identifier, args, e.type, e.loc), made_change};
}

std::pair<r_expr, bool> constant_fold(const resolved_object& e, std::unordered_map<std::string, r_expr>& constant_map) {
    std::vector<r_expr> values;
    bool made_change = false;
    for (const auto& a: e.record_values) {
        auto result = constant_fold(a, constant_map);
        values.push_back(result.first);
        made_change |= result.second;
    }
    return {make_rexpr<resolved_object>(e.r_identifier, e.record_fields, values, e.type, e.loc), made_change};
}

std::pair<r_expr, bool> constant_fold(const resolved_let& e, std::unordered_map<std::string, r_expr>& constant_map) {
    if (as_number(e.value)) {
        constant_map.insert({std::get<resolved_argument>(*e.identifier).name, e.value});
    }
    auto val  = constant_fold(e.value, constant_map);
    auto body = constant_fold(e.body, constant_map);
    return {make_rexpr<resolved_let>(e.identifier, val.first, body.first, e.type, e.loc), val.second||body.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_conditional& e,std::unordered_map<std::string, r_expr>& constant_map) {
    auto cond = constant_fold(e.condition, constant_map);
    auto tval = constant_fold(e.value_true, constant_map);
    auto fval = constant_fold(e.value_false, constant_map);

    if (auto val = as_number(cond.first)) {
        if ((bool)val.value()) {
            return {tval.first, true};
        }
        return {fval.first, true};
    }
    return {make_rexpr<resolved_conditional>(cond.first, tval.first, fval.first, e.type, e.loc),
            cond.second||tval.second||fval.second};
}

std::pair<r_expr, bool>  constant_fold(const resolved_float& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return {make_rexpr<resolved_float>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_int& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return {make_rexpr<resolved_int>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_unary& e, std::unordered_map<std::string, r_expr>& constant_map) {
    auto arg = constant_fold(e.arg, constant_map);
    if (auto val_opt = as_number(arg.first)) {
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
            return {make_rexpr<resolved_int>(val, e.type, e.loc), true};
        }
        return {make_rexpr<resolved_float>(val, e.type, e.loc), true};
    }
    return {make_rexpr<resolved_unary>(e.op, arg.first, e.type, e.loc), arg.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_binary& e, std::unordered_map<std::string, r_expr>& constant_map) {
    auto lhs_arg = constant_fold(e.lhs, constant_map);
    auto rhs_arg = constant_fold(e.rhs, constant_map);
    auto lhs_opt = as_number(lhs_arg.first);
    auto rhs_opt = as_number(rhs_arg.first);
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
                val = (bool)lhs && (bool)rhs; break;
            case binary_op::lor:
                val = (bool)lhs || (bool)rhs; break;
            case binary_op::min:
                val = std::min(lhs, rhs); break;
            case binary_op::max:
                val = std::max(lhs, rhs); break;
            case binary_op::dot:
                break;
        }
        if (is_integer(val)) {
            return {make_rexpr<resolved_int>(val, e.type, e.loc), true};
        }
        return {make_rexpr<resolved_float>(val, e.type, e.loc), true};
    }
    if (e.op == binary_op::dot) {
        std::string field;
        if (auto f_ptr = std::get_if<resolved_argument>(rhs_arg.first.get())) {
            field = f_ptr->name;
        }
        else {
            throw std::runtime_error("internal compiler error, expected argument name after `.` operator.");
        }

        if (auto o_ptr = std::get_if<resolved_object>(lhs_arg.first.get())) {
            int idx = -1;
            for (unsigned i = 0; i < o_ptr->record_fields.size(); ++i) {
                if (std::get<resolved_argument>(*(o_ptr->record_fields[i])).name == field) {
                    idx = (int)i;
                    break;
                }
            }
            if (idx < 0) {
                throw std::runtime_error("internal compiler error, expected to find field of object but failed.");
            }
            return {o_ptr->record_values[idx], true};
        }
    }
    return {make_rexpr<resolved_binary>(e.op, lhs_arg.first, rhs_arg.first, e.type, e.loc), lhs_arg.second||rhs_arg.second};
}

std::pair<r_expr, bool> constant_fold(const r_expr& e, std::unordered_map<std::string, r_expr>& constant_map) {
    return std::visit([&](auto& c) {return constant_fold(c, constant_map);}, *e);
}

std::pair<r_expr, bool> constant_fold(const r_expr& e) {
    std::unordered_map<std::string, r_expr> constant_map;
    return constant_fold(e, constant_map);
}

} // namespace resolved_ir
} // namespace al