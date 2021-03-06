#include <cmath>
#include <string>
#include <unordered_set>

#include <fmt/core.h>

#include <arblang/optimizer/constant_fold.hpp>
#include <arblang/util/custom_hash.hpp>

#include "../util/rexp_helpers.hpp"

namespace al {
namespace resolved_ir {

// Constant folding performs the following:
// 1. Propagates the values of resolved_constants,
//    and resolved_variables with constant values to
//    the expressions that use them. e.g.
//       constant a = 4.5;
//       let x = 3;
//       let r = a/x;
//    becomes:
//       constant a = 4.5;
//       let x = 3;
//       let r = 4.5/3;
// 2. Solves binary_expressions, unary_expressions and
//    conditional_expressions if their arguments have
//    constant values. The above example thus becomes:
//       constant a = 4.5;
//       let x = 3;
//       let r = 1.5;
// 3. Simplifies a binary_expression when only one of
//    its arguments has a constant value. e.g.
//    multiplication by, 1 or zero, addition of 0 etc.
// 4. Propagates the value of an object field with
//    to the expressions that access that field. e.g.
//       let a = {x = 4; y = _t0;};
//       let b = a.x + a.y;
//    becomes:
//       let a = {x = 4; y = _t0;};
//       let b = 4 + _t0;
//    TODO: propagating _t0 shouldn't be done in this  pass,
//          probably fits better in the copy propagation pass
//
// Constant folding needs to be performed in a loop,
// until no more changes can be made.

bool is_integer(double v) {
    return std::floor(v) == v;
}

std::pair<r_expr, bool> constant_fold(const resolved_record_alias& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation.");
}

std::pair<r_expr, bool> constant_fold(const resolved_argument& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    if (constant_map.count(e.name)) return {constant_map.at(e.name), true};
    return {make_rexpr<resolved_argument>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_variable& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    if (constant_map.count(e.name)) return {constant_map.at(e.name), true};
    if (rewrites.count(e.name)) return {rewrites.at(e.name), false};
    return {make_rexpr<resolved_variable>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_parameter& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto result = constant_fold(e.value, constant_map, rewrites);
    return {make_rexpr<resolved_parameter>(e.name, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_constant& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto result = constant_fold(e.value, constant_map, rewrites);
    return {make_rexpr<resolved_constant>(e.name, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_state& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    return {make_rexpr<resolved_state>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_function& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto result = constant_fold(e.body, constant_map, rewrites);
    return {make_rexpr<resolved_function>(e.name, e.args, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_bind& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    return {make_rexpr<resolved_bind>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_initial& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto result = constant_fold(e.value, constant_map, rewrites);
    return {make_rexpr<resolved_initial>(e.identifier, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_on_event& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto result = constant_fold(e.value, constant_map, rewrites);
    return {make_rexpr<resolved_on_event>(e.argument, e.identifier, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_evolve& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto result = constant_fold(e.value, constant_map, rewrites);
    return {make_rexpr<resolved_evolve>(e.identifier, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_effect& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto result = constant_fold(e.value, constant_map, rewrites);
    return {make_rexpr<resolved_effect>(e.effect, e.ion, result.first, e.type, e.loc), result.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_export& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    return {make_rexpr<resolved_export>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_call& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    std::vector<r_expr> args;
    bool made_change = false;
    for (const auto& a: e.call_args) {
        auto result = constant_fold(a, constant_map, rewrites);
        args.push_back(result.first);
        made_change |= result.second;
    }
    return {make_rexpr<resolved_call>(e.f_identifier, args, e.type, e.loc), made_change};
}

std::pair<r_expr, bool> constant_fold(const resolved_object& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    std::vector<r_expr> values;
    bool made_change = false;
    for (const auto& a: e.field_values()) {
        auto result = constant_fold(a, constant_map, rewrites);
        values.push_back(result.first);
        made_change |= result.second;
    }
    return {make_rexpr<resolved_object>(e.field_names(), values, e.type, e.loc), made_change};
}

std::pair<r_expr, bool> constant_fold(const resolved_let& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto var_name = e.id_name();
    if (is_number(e.id_value())) {
        constant_map.insert({var_name, e.id_value()});
    }

    auto val = constant_fold(e.id_value(), constant_map, rewrites);

    auto var_cst = make_rexpr<resolved_variable>(var_name, val.first, type_of(val.first), location_of(val.first));
    rewrites.insert({var_name, var_cst});

    auto body = constant_fold(e.body, constant_map, rewrites);
    return {make_rexpr<resolved_let>(var_cst, body.first, e.type, e.loc), val.second||body.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_conditional& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto cond = constant_fold(e.condition, constant_map, rewrites);
    auto tval = constant_fold(e.value_true, constant_map, rewrites);
    auto fval = constant_fold(e.value_false, constant_map, rewrites);

    if (auto val = is_number(cond.first)) {
        if ((bool)val.value()) {
            return {tval.first, true};
        }
        return {fval.first, true};
    }
    return {make_rexpr<resolved_conditional>(cond.first, tval.first, fval.first, e.type, e.loc),
            cond.second||tval.second||fval.second};
}

std::pair<r_expr, bool>  constant_fold(const resolved_float& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    return {make_rexpr<resolved_float>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_int& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    return {make_rexpr<resolved_int>(e), false};
}

std::pair<r_expr, bool> constant_fold(const resolved_unary& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto arg = constant_fold(e.arg, constant_map, rewrites);
    if (auto val_opt = is_number(arg.first)) {
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

std::pair<r_expr, bool> constant_fold(const resolved_binary& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto lhs_arg = constant_fold(e.lhs, constant_map, rewrites);
    auto rhs_arg = constant_fold(e.rhs, constant_map, rewrites);
    auto lhs_opt = is_number(lhs_arg.first);
    auto rhs_opt = is_number(rhs_arg.first);

    auto lhs_ptr = is_resolved_quantity_type(type_of(lhs_arg.first));
    bool lhs_real = lhs_ptr && lhs_ptr->type.is_real();

    auto rhs_ptr = is_resolved_quantity_type(type_of(rhs_arg.first));
    bool rhs_real = rhs_ptr && rhs_ptr->type.is_real();

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
    else if (lhs_opt) {
        auto lhs = lhs_opt.value();
        if (lhs == 0) {
            switch (e.op) {
                case binary_op::add:  return {rhs_arg.first, true};
                case binary_op::sub:  return {make_rexpr<resolved_unary>(unary_op::neg, rhs_arg.first, e.type, e.loc), true};
                case binary_op::mul:  return {make_rexpr<resolved_int>(0, e.type, e.loc), true};
                case binary_op::div:  return {make_rexpr<resolved_int>(0, e.type, e.loc), true};
                case binary_op::land: return {make_rexpr<resolved_int>(0, e.type, e.loc), true};
                case binary_op::lor:  return {rhs_arg.first, true};
                case binary_op::pow:  return {make_rexpr<resolved_int>(0, e.type, e.loc), true};
                default: break;
            }
        }
        else if (lhs == 1) {
            switch (e.op) {
                case binary_op::land: return {rhs_arg.first, true};
                case binary_op::lor:  return {make_rexpr<resolved_int>(1, e.type, e.loc), true};
                case binary_op::pow:  return {make_rexpr<resolved_int>(1, e.type, e.loc), true};
                case binary_op::mul:  if (lhs_real) return {rhs_arg.first, true};
                default: break;
            }
        }
    }
    else if (rhs_opt) {
        auto rhs = rhs_opt.value();
        if (rhs == 0) {
            switch (e.op) {
                case binary_op::add: return {lhs_arg.first, true};
                case binary_op::sub: return {lhs_arg.first, true};
                case binary_op::mul: return {make_rexpr<resolved_int>(0, e.type, e.loc), true};
                case binary_op::div: throw std::runtime_error(fmt::format("Divide by zero detected at {}.",
                                                                          to_string(location_of(e.lhs))));
                case binary_op::land: return {make_rexpr<resolved_int>(0, e.type, e.loc), true};
                case binary_op::lor:  return {lhs_arg.first, true};
                case binary_op::pow:  return {make_rexpr<resolved_int>(1, e.type, e.loc), true};
                default: break;
            }
        }
        else if (rhs == 1) {
            switch (e.op) {
                case binary_op::land: return {lhs_arg.first, true};
                case binary_op::lor:  return {make_rexpr<resolved_int>(1, e.type, e.loc), true};
                case binary_op::pow:  return {lhs_arg.first, true};
                case binary_op::mul:  if (rhs_real) return {lhs_arg.first, true};
                case binary_op::div:  if (rhs_real) return {lhs_arg.first, true};
                default: break;
            }
        } else {
            // transform divisions by constant into multiplications by constant.
            if (e.op == binary_op::div) {
                if (auto q = is_resolved_quantity_type(type_of(e.rhs))) {
                    auto q_inv = normalized_type(quantity::real) / q->type;
                    auto rhs_inv = make_rexpr<resolved_float>(1/rhs, make_rtype<resolved_quantity>(q_inv, q->loc), location_of(e.rhs));
                    return {make_rexpr<resolved_binary>(binary_op::mul, e.lhs, rhs_inv, e.type, e.loc), true};
                } else {
                    throw std::runtime_error(fmt::format("Internal compiler error: unexpected type or rhs argument of {} op at {}.",
                                                         to_string(e.op), to_string(e.loc)));
                }
            }
        }
    }
    else {
        if (*lhs_arg.first == *rhs_arg.first) {
            switch (e.op) {
                case binary_op::sub: return {make_rexpr<resolved_int>(0, e.type, e.loc), true};
                case binary_op::div: return {make_rexpr<resolved_int>(1, e.type, e.loc), true};
                case binary_op::lt:  return {make_rexpr<resolved_int>(0, e.type, e.loc), true};
                case binary_op::le:  return {make_rexpr<resolved_int>(1, e.type, e.loc), true};
                case binary_op::gt:  return {make_rexpr<resolved_int>(0, e.type, e.loc), true};
                case binary_op::ge:  return {make_rexpr<resolved_int>(1, e.type, e.loc), true};
                case binary_op::eq:  return {make_rexpr<resolved_int>(1, e.type, e.loc), true};
                case binary_op::ne:  return {make_rexpr<resolved_int>(0, e.type, e.loc), true};
                case binary_op::min: return {lhs_arg.first, true};
                case binary_op::max: return {lhs_arg.first, true};
                default: break;
            }
        }
    }
    return {make_rexpr<resolved_binary>(e.op, lhs_arg.first, rhs_arg.first, e.type, e.loc), lhs_arg.second||rhs_arg.second};
}

std::pair<r_expr, bool> constant_fold(const resolved_field_access& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    auto obj_arg = constant_fold(e.object, constant_map, rewrites);
    auto field = e.field;

    // TODO This shouldn't be in the constant fold pass
    if (auto o_ptr = is_resolved_object(obj_arg.first)) {
        int idx = -1;
        for (unsigned i = 0; i < o_ptr->record_fields.size(); ++i) {
            if (is_resolved_variable(o_ptr->record_fields[i])->name == field) {
                idx = (int)i;
                break;
            }
        }
        if (idx < 0) {
            throw std::runtime_error("internal compiler error, expected to find field of object but failed.");
        }
        return {o_ptr->field_values()[idx], true};
    }
    return {make_rexpr<resolved_field_access>(obj_arg.first, field, e.type, e.loc), obj_arg.second};
}

std::pair<resolved_mechanism, bool> constant_fold(const resolved_mechanism& e) {
    std::unordered_map<std::string, r_expr> constants_map, rewrites, local_constant_map;
    std::unordered_set<std::string> exported_params;

    auto reset_maps = [&]() {
        rewrites.clear();
        local_constant_map.clear();
        local_constant_map = constants_map;
    };

    resolved_mechanism mech;
    bool made_changes = false;
    // Parameters that are not exported can be constant propagated.
    for (const auto& c: e.exports) {
        // No need to constant fold, there is nothing to do.
        mech.exports.push_back(c);

        // Keep set of exported parameters.
        // Remaining un-exported parameters can be constant propagated.
        auto param_id = is_resolved_export(c)->identifier;
        exported_params.insert(is_resolved_argument(param_id)->name);
    }
    for (const auto& c: e.constants) {
        reset_maps();
        auto result = constant_fold(c, local_constant_map, rewrites);

        auto constant  = is_resolved_constant(result.first);
        if (is_number(constant->value)) {
            constants_map.insert({constant->name, constant->value});
        } else {
            mech.constants.push_back(result.first);
        }
        made_changes |= result.second;
    }
    for (const auto& c: e.parameters) {
        reset_maps();
        auto result = constant_fold(c, local_constant_map, rewrites);

        auto param  = is_resolved_parameter(result.first);
        if (!exported_params.count(param->name) && is_number(param->value)) {
            constants_map.insert({param->name, param->value});
        } else {
            mech.parameters.push_back(result.first);
        }
        made_changes |= result.second;
    }
    for (const auto& c: e.bindings) {
        reset_maps();
        auto result = constant_fold(c, local_constant_map, rewrites);
        mech.bindings.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.states) {
        reset_maps();
        auto result = constant_fold(c, local_constant_map, rewrites);
        mech.states.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.functions) {
        reset_maps();
        auto result = constant_fold(c, local_constant_map, rewrites);
        mech.functions.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.initializations) {
        reset_maps();
        auto result = constant_fold(c, local_constant_map, rewrites);
        mech.initializations.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.on_events) {
        reset_maps();
        auto result = constant_fold(c, local_constant_map, rewrites);
        mech.on_events.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.evolutions) {
        reset_maps();
        auto result = constant_fold(c, local_constant_map, rewrites);
        mech.evolutions.push_back(result.first);
        made_changes |= result.second;
    }
    for (const auto& c: e.effects) {
        reset_maps();
        auto result = constant_fold(c, local_constant_map, rewrites);
        mech.effects.push_back(result.first);
        made_changes |= result.second;
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return {mech, made_changes};
}

std::pair<r_expr, bool> constant_fold(const r_expr& e,
                                      std::unordered_map<std::string, r_expr>& constant_map,
                                      std::unordered_map<std::string, r_expr>& rewrites)
{
    return std::visit([&](auto& c) {return constant_fold(c, constant_map, rewrites);}, *e);
}

std::pair<r_expr, bool> constant_fold(const r_expr& e) {
    std::unordered_map<std::string, r_expr> constant_map, rewrites;
    return constant_fold(e, constant_map, rewrites);
}

} // namespace resolved_ir
} // namespace al