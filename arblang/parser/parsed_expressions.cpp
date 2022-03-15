#include <cassert>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include <arblang/parser/parsed_expressions.hpp>
#include <arblang/util/common.hpp>

namespace al {
namespace parsed_ir {
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

// parsed_mechanism
bool parsed_mechanism::set_kind(tok t) {
    if (auto k = gen_mechanism_kind(t)) {
        kind = k.value();
        return true;
    }
    return false;
}

// parsed_bind
parsed_bind::parsed_bind(p_expr iden, const token& t, const std::string& ion_name, const src_location& loc):
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

// parsed_effect
parsed_effect::parsed_effect(const token& t, const std::string& ion_name, std::optional<parsed_type_ir::p_type> type, p_expr value, const src_location& loc):
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

// parsed_unary
parsed_unary::parsed_unary(tok t, p_expr value, const src_location& loc):
    value(std::move(value)), loc(loc) {
    if (auto uop = gen_unary_op(t)) {
        op = uop.value();
    } else {
        throw std::runtime_error("Unexpected unary operator token");
    };
}

bool parsed_unary::is_boolean () const {
    return op == unary_op::lnot;
}

// parsed_binary
parsed_binary::parsed_binary(tok t, p_expr lhs, p_expr rhs, const src_location& loc):
    lhs(std::move(lhs)), rhs(std::move(rhs)), loc(loc) {
    if (auto bop = gen_binary_op(t)) {
        op = bop.value();
    } else {
        throw std::runtime_error("Unexpected binary operator token");
    };
}

bool parsed_binary::is_boolean () const {
    return (op == binary_op::land) || (op == binary_op::lor) || (op == binary_op::ge) ||
           (op == binary_op::gt)   || (op == binary_op::le)  || (op == binary_op::lt) ||
           (op == binary_op::eq)   || (op == binary_op::ne);
}

// to_string

std::string to_string(const parsed_mechanism& e, int indent) {
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

// parsed_parameter
std::string to_string(const parsed_parameter& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_parameter\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

// parsed_constant
std::string to_string(const parsed_constant& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_constant\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

// parsed_state
std::string to_string(const parsed_state& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_state\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

// parsed_record_alias
std::string to_string(const parsed_record_alias& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_record_alias\n";
    str += double_indent + e.name + "\n";
    str += to_string(e.type, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

// parsed_function
std::string to_string(const parsed_function& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_function\n";
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

std::string to_string(const parsed_bind& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_bind\n";
    str += (double_indent + to_string(e.bind));
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_initial& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_initial\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_evolve& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_evolve\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_effect& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_effect\n";
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

std::string to_string(const parsed_export& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_export\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_call& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_call\n";
    str += double_indent + e.function_name + "\n";
    for (const auto& f: e.call_args) {
        str += to_string(f, indent+1) + "\n";
    }
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_object& e, int indent) {
    assert(e.record_fields.size() == e.record_values.size());

    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_object\n";
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

std::string to_string(const parsed_let& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_let\n";
    str += to_string(e.identifier, indent+1) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += to_string(e.body, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_with& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";
    std::string str = single_indent + "(parsed_with\n";
    str += to_string(e.value, indent+1) + "\n";
    str += to_string(e.body, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_conditional& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_conditional\n";
    str += to_string(e.condition, indent+1) + "\n";
    str += to_string(e.value_true, indent+1) + "\n";
    str += to_string(e.value_false, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_identifier& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_identifier \n";
    str += (double_indent + e.name + "\n");
    if (e.type) {
        str += to_string(e.type.value(), indent+1) + "\n";
    }
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_float& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_float\n";
    str += double_indent + std::to_string(e.value) + "\n";
    str += to_string(e.unit, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_int& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_int\n";
    str += double_indent + std::to_string(e.value) + "\n";
    str += to_string(e.unit, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_unary& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_unary " + to_string(e.op) + "\n";
    str += to_string(e.value, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const parsed_binary& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_binary " + to_string(e.op) + "\n";
    str += to_string(e.lhs, indent+1) + "\n";
    str += to_string(e.rhs, indent+1) + "\n";
    str += double_indent + to_string(e.loc) + ")";
    return str;
}

std::string to_string(const p_expr& e, int indent) {
    return std::visit([&](auto&& c){return to_string(c, indent);}, *e);
}

src_location location_of(const p_expr& e) {
    return std::visit([](auto&& c){return c.loc;}, *e);
}

} // namespace al
} // namespace parsed_ir
