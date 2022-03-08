#pragma once

#include <string>

namespace al {

enum class binary_op {
    add, sub, mul, div, pow,
    lt, le, gt, ge, eq, ne,
    land, lor, min, max, dot
};

enum class unary_op {
    exp, log, cos, sin, abs, exprelr, lnot, neg
};

enum class mechanism_kind {
    density, point, concentration, junction,
};

enum class bindable {
    membrane_potential, temperature, current_density,
    molar_flux, charge,
    internal_concentration, external_concentration, nernst_potential,
    dt
};

enum class affectable {
    current_density, current, molar_flux, molar_flow_rate,
    internal_concentration_rate, external_concentration_rate,
    current_density_pair, // current density + conductivity
    current_pair          // current + conductance
};

static std::string to_string(const binary_op& op) {
    switch (op) {
        case binary_op::add:  return "+";
        case binary_op::sub:  return "-";
        case binary_op::mul:  return "*";
        case binary_op::div:  return "/";
        case binary_op::pow:  return "^";
        case binary_op::ne:   return "!=";
        case binary_op::lt:   return "<";
        case binary_op::le:   return "<=";
        case binary_op::gt:   return ">";
        case binary_op::ge:   return ">=";
        case binary_op::land: return "&&";
        case binary_op::lor:  return "||";
        case binary_op::eq:   return "==";
        case binary_op::max:  return "max";
        case binary_op::min:  return "min";
        case binary_op::dot:  return ".";
        default: return {};
    }
}
static std::string to_string(const unary_op& op) {
    switch (op) {
        case unary_op::exp:     return "exp";
        case unary_op::exprelr: return "exprelr";
        case unary_op::log:     return "log";
        case unary_op::cos:     return "cos";
        case unary_op::sin:     return "sin";
        case unary_op::abs:     return "abs";
        case unary_op::lnot:    return "lnot";
        case unary_op::neg:     return "-";
        default: return {};
    }
}

static std::string to_string(const mechanism_kind& op) {
    switch (op) {
        case mechanism_kind::density:       return "denity";
        case mechanism_kind::concentration: return "concentration";
        case mechanism_kind::junction:      return "junction";
        case mechanism_kind::point:         return "point";
        default: return {};
    }
}

static std::string to_string(const bindable& op) {
    switch (op) {
        case bindable::membrane_potential:     return "membrane_potential";
        case bindable::temperature:            return "temperature";
        case bindable::current_density:        return "current_density";
        case bindable::molar_flux:             return "molar_flux";
        case bindable::charge:                 return "charge";
        case bindable::internal_concentration: return "internal_concentration";
        case bindable::external_concentration: return "external_concentration";
        case bindable::nernst_potential:       return "nernst_potential";
        default: return {};
    }
}

static std::string to_string(const affectable& op) {
    switch (op) {
        case affectable::current_density:             return "current_density";
        case affectable::current:                     return "current";
        case affectable::molar_flux:                  return "molar_flux";
        case affectable::molar_flow_rate:             return "molar_flow_rate";
        case affectable::internal_concentration_rate: return "internal_concentration_rate";
        case affectable::external_concentration_rate: return "external_concentration_rate";
        default: return {};
    }
}

} // namespace al