#include <cassert>
#include <optional>
#include <sstream>
#include <string>

#include <arblang/raw_expressions.hpp>
#include <utility>

namespace al {
namespace raw_ir {
std::optional<binary_op> gen_binary_op(tok t) {
    switch (t) {
        case tok::plus:     return binary_op::add;
        case tok::minus:    return binary_op::sub;
        case tok::times:    return binary_op::mul;
        case tok::divide:   return binary_op::div;
        case tok::pow:      return binary_op::pow;
        case tok::ne:       return binary_op::ne;
        case tok::lt:       return binary_op::lt;
        case tok::le:       return binary_op::le;
        case tok::gt:       return binary_op::gt;
        case tok::ge:       return binary_op::ge;
        case tok::land:     return binary_op::land;
        case tok::lor:      return binary_op::lor;
        case tok::equality: return binary_op::eq;
        case tok::max:      return binary_op::max;
        case tok::min:      return binary_op::min;
        case tok::dot:      return binary_op::dot;
        default: return {};
    }
}
std::optional<unary_op> gen_unary_op(tok t) {
    switch (t) {
        case tok::exp:     return unary_op::exp;
        case tok::exprelr: return unary_op::exprelr;
        case tok::log:     return unary_op::log;
        case tok::cos:     return unary_op::cos;
        case tok::sin:     return unary_op::sin;
        case tok::abs:     return unary_op::abs;
        case tok::lnot:    return unary_op::lnot;
        case tok::minus:   return unary_op::neg;
        default: return {};
    }
}

std::optional<mechanism_kind> gen_mechanism_kind(tok t) {
    switch (t) {
        case tok::density:       return mechanism_kind::density;
        case tok::concentration: return mechanism_kind::concentration;
        case tok::junction:      return mechanism_kind::junction;
        case tok::point:         return mechanism_kind::point;
        default: return {};
    }
}

std::optional<bindable> gen_bindable(tok t) {
    switch (t) {
        case tok::membrane_potential:     return bindable::membrane_potential;
        case tok::temperature:            return bindable::temperature;
        case tok::current_density:        return bindable::current_density;
        case tok::molar_flux:             return bindable::molar_flux;
        case tok::charge:                 return bindable::charge;
        case tok::internal_concentration: return bindable::internal_concentration;
        case tok::external_concentration: return bindable::external_concentration;
        case tok::nernst_potential:       return bindable::nernst_potential;
        default: return {};
    }
}

std::optional<affectable> gen_affectable(tok t) {
    switch (t) {
        case tok::current_density:             return affectable::current_density;
        case tok::current:                     return affectable::current;
        case tok::molar_flux:                  return affectable::molar_flux;
        case tok::molar_flow_rate:             return affectable::molar_flow_rate;
        case tok::internal_concentration_rate: return affectable::internal_concentration_rate;
        case tok::external_concentration_rate: return affectable::external_concentration_rate;
        default: return {};
    }
}

std::string to_string(const binary_op& op) {
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
std::string to_string(const unary_op& op) {
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

std::string to_string(const mechanism_kind& op) {
    switch (op) {
        case mechanism_kind::density:       return "denity";
        case mechanism_kind::concentration: return "concentration";
        case mechanism_kind::junction:      return "junction";
        case mechanism_kind::point:         return "point";
        default: return {};
    }
}

std::string to_string(const bindable& op) {
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

std::string to_string(const affectable& op) {
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

// mechanism_expr
bool mechanism_expr::set_kind(tok t) {
    if (auto k = gen_mechanism_kind(t)) {
        kind = k.value();
        return true;
    }
    return false;
}

std::string to_string(const mechanism_expr& e, int indent) {
    auto indent_str = std::string(indent*2, ' ');
    std::string str = indent_str + "(module_expr " + e.name + " " + to_string(e.kind) + "\n";
    for (const auto& p: e.parameters) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.constants) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.states) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.bindings) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.functions) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.records) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.initilizations) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.evolutions) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.effects) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    for (const auto& p: e.exports) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *p);
    }
    return str + to_string(e.loc) + ")";
}

// parameter_expr
std::string to_string(const parameter_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parameter_expr\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    return str + double_indent + to_string(e.loc) + ")";
}

// constant_expr
std::string to_string(const constant_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(constant_expr\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    return str + double_indent + to_string(e.loc) + ")";
}

// state_expr
std::string to_string(const state_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(state_expr\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    return str + double_indent + to_string(e.loc) + ")";
}

// record_alias_expr
std::string to_string(const record_alias_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(record_alias_expr\n";
    str += (double_indent + e.name + "\n");
    std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *(e.type));
    return str + double_indent + to_string(e.loc) + ")";
}

// function_expr
std::string to_string(const function_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(function_expr\n";
    str += (double_indent + e.name +  "\n");
    if (e.ret) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *(e.ret.value()));
    }

    str += (double_indent + "(\n");
    for (const auto& f: e.args) {
        std::visit([&](auto&& c){str += (to_string(c, indent+2) + "\n");}, *f);
    }
    str += (double_indent + ")\n");
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.body);
    return str + double_indent + to_string(e.loc) + ")";
}

// bind_expr
bind_expr::bind_expr(expr iden, const token& t, const std::string& ion_name, const src_location& loc):
    identifier(std::move(iden)), loc(loc)
{
    if (auto b = gen_bindable(t.type)) {
        bind = b.value();
        if (!ion_name.empty()) {
            if (!t.ion_bindable()) {
                throw std::runtime_error("Generating non-ion bindable with an ion: internal compiler error");
            }
            ion = ion_name;
        } else {
            if (t.ion_bindable()) {
                throw std::runtime_error("Generating ion bindable without an ion: internal compiler error");
            }
        }
    } else {
        throw std::runtime_error("Expected a valid bindable: internal compiler error");
    }
};

std::string to_string(const bind_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(bind_expr\n";
    str += (double_indent + to_string(e.bind));
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    return str + double_indent + to_string(e.loc) + ")";
}

// initial_expr
std::string to_string(const initial_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(initial_expr\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    return str + double_indent + to_string(e.loc) + ")";
}

// evolve_expr
std::string to_string(const evolve_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(evolve_expr\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    return str + double_indent + to_string(e.loc) + ")";
}

// effect_expr
effect_expr::effect_expr(const token& t, const std::string& ion_name, std::optional<t_raw_ir::t_expr> type, expr value, const src_location& loc):
        value(std::move(value)), type(std::move(type)), loc(loc)
{
    if (auto b = gen_affectable(t.type)) {
        effect = b.value();
        if (!ion_name.empty()) {
            ion = ion_name;
        } else {
            if (effect != affectable::current && effect != affectable::current_density) {
                throw std::runtime_error("Generating ion effect without an ion: internal compiler error");
            }
        }
    } else {
        throw std::runtime_error("Expected a valid effect: internal compiler error");
    }
};
std::string to_string(const effect_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(effect_expr\n";
    str += (double_indent + to_string(e.effect));
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    if (e.type) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.type.value());
    }
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    return str + double_indent + to_string(e.loc) + ")";
}

// export_expr
std::string to_string(const export_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(export_expr\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    return str + double_indent + to_string(e.loc) + ")";
}

// call_expr
std::string to_string(const call_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(call_expr\n";
    str += (double_indent + e.function_name + "\n");
    for (const auto& f: e.call_args) {
        std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *f);
    }
    return str + double_indent + to_string(e.loc) + ")";
}

// object_expr
std::string to_string(const object_expr& e, int indent) {
    assert(e.record_fields.size() == e.record_values.size());

    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(object_expr\n";
    if (e.record_name) str += (double_indent + e.record_name.value() +  "\n");
    for (unsigned i = 0; i < e.record_fields.size(); ++i) {
        str += (double_indent + "(\n");
        std::visit([&](auto&& c){str += (to_string(c, indent+2) + "\n");}, *(e.record_fields[i]));
        std::visit([&](auto&& c){str += (to_string(c, indent+2) + "\n");}, *(e.record_values[i]));
        str += (double_indent + ")\n");
    }
    return str + double_indent + to_string(e.loc) + ")";
}

// let_expr
std::string to_string(const let_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(let_expr\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.identifier);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.body);
    return str + double_indent + to_string(e.loc) + ")";
}

// with_expr
std::string to_string(const with_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";
    std::string str = single_indent + "(with_expr\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.body);
    return str + double_indent + to_string(e.loc) + ")";
}

// conditional_expr
std::string to_string(const conditional_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(conditional_expr\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.condition);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value_true);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value_false);
    return str + double_indent + to_string(e.loc) + ")";
}

// identifier_expr
std::string to_string(const identifier_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(identifier_expr \n";
    str += (double_indent + e.name + "\n");
    if (e.type) {
        std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *(e.type.value()));
    }
    return str + double_indent + to_string(e.loc) + ")";
}

// float_expr
std::string to_string(const float_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(float_expr\n";
    str += (double_indent + std::to_string(e.value) + "\n");
    if (e.unit) {
        std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *(e.unit.value()));
    }
    return str + double_indent + to_string(e.loc) + ")";
}

// int_expr
std::string to_string(const int_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(int_expr\n";
    str += (double_indent + std::to_string(e.value) + "\n");
    if (e.unit) {
        std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *(e.unit.value()));
    }
    return str + double_indent + to_string(e.loc) + ")";
}

// unary_expr
unary_expr::unary_expr(tok t, expr value, const src_location& loc):
    value(std::move(value)), loc(loc) {
    if (auto uop = gen_unary_op(t)) {
        op = uop.value();
    } else {
        throw std::runtime_error("Unexpected unary operator token");
    };
}

bool unary_expr::is_boolean () const {
    return op == unary_op::lnot;
}

std::string to_string(const unary_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(unary_expr " + to_string(e.op) + "\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.value);
    return str + double_indent + to_string(e.loc) + ")";
}

// binary_expr
binary_expr::binary_expr(tok t, expr lhs, expr rhs, const src_location& loc):
    lhs(std::move(lhs)), rhs(std::move(rhs)), loc(loc) {
    if (auto bop = gen_binary_op(t)) {
        op = bop.value();
    } else {
        throw std::runtime_error("Unexpected binary operator token");
    };
}

bool binary_expr::is_boolean () const {
    return (op == binary_op::land) || (op == binary_op::lor) || (op == binary_op::ge) ||
           (op == binary_op::gt)   || (op == binary_op::le)  || (op == binary_op::lt) ||
           (op == binary_op::eq)   || (op == binary_op::ne);
}

std::string to_string(const binary_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(binary_expr " + to_string(e.op) + "\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.lhs);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *e.rhs);
    return str + double_indent + to_string(e.loc) + ")";
}

} // namespace al
} // namespace raw_ir
