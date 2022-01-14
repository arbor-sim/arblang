#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <arblang/common.hpp>
#include <arblang/token.hpp>
#include <arblang/type_expressions.hpp>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/compile.h>

namespace al {
namespace t_raw_ir {
std::optional<quantity> gen_quantity(tok t) {
    switch (t) {
        case tok::real: return quantity::real;
        case tok::length:        return quantity::length;
        case tok::mass:          return quantity::mass;
        case tok::time:          return quantity::time;
        case tok::current:       return quantity::current;
        case tok::amount:        return quantity::amount;
        case tok::temperature:   return quantity::temperature;
        case tok::charge:        return quantity::charge;
        case tok::frequency:     return quantity::frequency;
        case tok::voltage:       return quantity::voltage;
        case tok::resistance:    return quantity::resistance;
        case tok::conductance:   return quantity::conductance;
        case tok::capacitance:   return quantity::capacitance;
        case tok::inductance:    return quantity::inductance;
        case tok::force:         return quantity::force;
        case tok::pressure:      return quantity::pressure;
        case tok::energy:        return quantity::energy;
        case tok::power:         return quantity::power;
        case tok::area:          return quantity::area;
        case tok::volume:        return quantity::volume;
        case tok::concentration: return quantity::concentration;
        default: return {};
    }
}

std::optional<t_binary_op> gen_binary_op(tok t) {
    switch (t) {
        case tok::times:
            return t_binary_op::mul;
        case tok::divide:
            return t_binary_op::div;
        case tok::pow:
            return t_binary_op::pow;
        default: return {};
    }
}

//quantity_type;
quantity_type::quantity_type(tok t, src_location loc): loc(loc) {
    if (auto q = gen_quantity(t)) {
        type = q.value();
    } else {
        throw std::runtime_error("Unexpected unary operator token");
    };
};

//quantity_binary_type;
quantity_binary_type::quantity_binary_type(t_binary_op op, t_expr l, t_expr r, const src_location& location):
    op(op), lhs(std::move(l)), rhs(std::move(r)), loc(location)
{
    if (!verify()) {
        throw std::runtime_error(fmt::format("Invalid quantity expression at {}", to_string(location)));
    }
}

quantity_binary_type::quantity_binary_type(tok t, t_expr l, t_expr r, const src_location& location):
    lhs(std::move(l)), rhs(std::move(r)), loc(location)
{
    if (auto t_op = gen_binary_op(t)) {
        op = t_op.value();
    } else {
        throw std::runtime_error(fmt::format("Invalid quantity expression operator at {}", to_string(location)));
    }
    if (!verify()) {
        throw std::runtime_error(fmt::format("Invalid quantity expression at {}", to_string(location)));
    }
}

bool quantity_binary_type::verify() const {
    auto is_int  = [](const t_expr& t) {return std::get_if<integer_type>(t.get());};
    auto is_pow = (op == t_binary_op::pow);

    if (is_int(lhs)) return false;
    if ((is_pow && !is_int(rhs)) || (!is_pow && is_int(rhs))) return false;

    auto is_allowed = [](const t_expr& t) {
        return std::visit(al::util::overloaded {
                [&](const boolean_type& t)         {return false;},
                [&](const record_type& t)          {return false;},
                [&](const record_alias_type& t)    {return false;},
                [&](const quantity_type& t)        {return true;},
                [&](const integer_type& t)         {return true;},
                [&](const quantity_binary_type& t) {return true;}
        }, *t);
    };
    if (!is_allowed(lhs) || !is_allowed(rhs)) return false;
    return true;
}

// to_string
std::string to_string(const t_binary_op& op) {
    switch(op) {
        case t_binary_op::mul: return "*";
        case t_binary_op::div: return "/";
        case t_binary_op::pow: return "^";
        default: return {};
    }
}

std::string to_string(quantity q) {
    switch(q) {
        case quantity::real:          return "real";
        case quantity::length:        return "length";
        case quantity::mass:          return "mass";
        case quantity::time:          return "time";
        case quantity::current:       return "current";
        case quantity::amount:        return "amount";
        case quantity::temperature:   return "temperature";
        case quantity::charge:        return "charge";
        case quantity::frequency:     return "frequency";
        case quantity::voltage:       return "voltage";
        case quantity::resistance:    return "resistance";
        case quantity::conductance:   return "conductance";
        case quantity::capacitance:   return "capacitance";
        case quantity::inductance:    return "inductance";
        case quantity::force:         return "force";
        case quantity::pressure:      return "pressure";
        case quantity::energy:        return "energy";
        case quantity::power:         return "power";
        case quantity::area:          return "area";
        case quantity::volume:        return "volume";
        case quantity::concentration: return "concentration";
        default: return {};
    }
}

std::string to_string(const integer_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent +  "(integer_type\n";
    str += (double_indent + std::to_string(q.val) + "\n");
    return str + double_indent + to_string(q.loc) + ")";
}

std::string to_string(const quantity_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(quantity_type\n";
    str += double_indent + to_string(q.type) + "\n";
    str += double_indent + to_string(q.loc) + ")";
    return str;
}

std::string to_string(const quantity_binary_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(quantity_binary_type " + to_string(q.op) + "\n";
    str += to_string(q.lhs, indent+1) + "\n";
    str += to_string(q.rhs, indent+1) + "\n";
    str += double_indent + to_string(q.loc) + ")";
    return str;
}

std::string to_string(const boolean_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    return single_indent + "(boolean_type " + to_string(q.loc) + ")";
}

std::string to_string(const record_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent +  "(record_type\n";
    for (const auto& f: q.fields) {
        str += to_string(f.second, indent+1) + "\n";
    }
    str += double_indent + to_string(q.loc) + ")";
    return str;
}

std::string to_string(const record_alias_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(record_alias_type\n";
    str += (double_indent + q.name + "\n");
    str += double_indent + to_string(q.loc) + ")";
    return str;
}

std::string to_string(const t_expr& t, int indent) {
    return std::visit([&](auto&& c) {return to_string(c, indent);}, *t);
}

} // namespace t_raw_ir
} // namespace al