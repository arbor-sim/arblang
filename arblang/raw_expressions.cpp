#include <cassert>
#include <optional>
#include <sstream>
#include <string>

#include <arblang/common.hpp>
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

// mechanism_expr
bool mechanism_expr::set_kind(tok t) {
    if (auto k = gen_mechanism_kind(t)) {
        kind = k.value();
        return true;
    }
    return false;
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

// to_string

std::string to_string(const mechanism_expr& e, int indent) {
    auto indent_str = std::string(indent*2, ' ');
    std::string str = indent_str + "(module_expr " + e.name + " " + to_string(e.kind) + "\n";
    for (const auto& p: e.parameters) {
        str += to_string(p, indent+1) + "\n";
    }
    for (const auto& p: e.constants) {
        str += to_string(p, indent+1) + "\n";
    }
    for (const auto& p: e.states) {
        str += to_string(p, indent+1) + "\n";
    }
    for (const auto& p: e.bindings) {
        str += to_string(p, indent+1) + "\n";
    }
    for (const auto& p: e.functions) {
        str += to_string(p, indent+1) + "\n";
    }
    for (const auto& p: e.records) {
        str += to_string(p, indent+1) + "\n";
    }
    for (const auto& p: e.initializations) {
        str += to_string(p, indent+1) + "\n";
    }
    for (const auto& p: e.evolutions) {
        str += to_string(p, indent+1) + "\n";
    }
    for (const auto& p: e.effects) {
        str += to_string(p, indent+1) + "\n";
    }
    for (const auto& p: e.exports) {
        str += to_string(p, indent+1) + "\n";
    }
    str += to_string(e.loc) + ")";
    return str;
}

// parameter_expr
std::string to_string(const parameter_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parameter_expr\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

// constant_expr
std::string to_string(const constant_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(constant_expr\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

// state_expr
std::string to_string(const state_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(state_expr\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

// record_alias_expr
std::string to_string(const record_alias_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(record_alias_expr\n";
    str += double_indent + e.name + "\n";
    str += to_string(e.type, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

// function_expr
std::string to_string(const function_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(function_expr\n";
    str += (double_indent + e.name +  "\n");
    if (e.ret) {
        str += to_string(e.ret.value(), indent+1) + "\n";
    }

    str += double_indent + "(\n";
    for (const auto& f: e.args) {
        str += to_string(f, indent+2) + "\n";
    }
    str += double_indent + ")\n";
    str += to_string(e.body, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const bind_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(bind_expr\n";
    str += (double_indent + to_string(e.bind));
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const initial_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(initial_expr\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const evolve_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(evolve_expr\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

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
        str += to_string(e.type.value(), indent+1) + "\n";
    }
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const export_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(export_expr\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const call_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(call_expr\n";
    str += double_indent + e.function_name + "\n";
    for (const auto& f: e.call_args) {
        str += to_string(f, indent+1) + "\n";
    }
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const object_expr& e, int indent) {
    assert(e.record_fields.size() == e.record_values.size());

    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(object_expr\n";
    if (e.record_name) str += (double_indent + e.record_name.value() +  "\n");
    for (unsigned i = 0; i < e.record_fields.size(); ++i) {
        str += double_indent + "(\n";
        str += to_string(e.record_fields[i], indent+2) + "\n";
        str += to_string(e.record_values[i], indent+2) + "\n";
        str += double_indent + ")\n";
    }
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const let_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(let_expr\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += to_string(e.body, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const with_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";
    std::string str = single_indent + "(with_expr\n";
    str += to_string(e.value, indent+1) + "\n";
    str += to_string(e.body, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const conditional_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(conditional_expr\n";
    str += to_string(e.condition, indent+1) + "\n";
    str += to_string(e.value_true, indent+1) + "\n";
    str += to_string(e.value_false, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const identifier_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(identifier_expr \n";
    str += (double_indent + e.name + "\n");
    if (e.type) {
        str += to_string(e.type.value(), indent+1) + "\n";
    }
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const float_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(float_expr\n";
    str += double_indent + std::to_string(e.value) + "\n";
    str += to_string(e.unit, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const int_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(int_expr\n";
    str += double_indent + std::to_string(e.value) + "\n";
    str += to_string(e.unit, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const unary_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(unary_expr " + to_string(e.op) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const binary_expr& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(binary_expr " + to_string(e.op) + "\n";
    str += to_string(e.lhs, indent+1) + "\n";
    str += to_string(e.rhs, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const expr& e, int indent) {
    return std::visit([&](auto&& c){return to_string(c, indent);}, *e);
}


} // namespace al
} // namespace raw_ir
