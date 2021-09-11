#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <arblang/token.hpp>
#include <arblang/type_expressions.hpp>

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
        case tok::force:         return quantity::force;
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

std::ostream& operator<< (std::ostream& o, const t_binary_op& op) {
    switch(op) {
        case t_binary_op::mul: return o << "*";
        case t_binary_op::div: return o << "/";
        case t_binary_op::pow: return o << "^";
        default: return o;
    }
}

std::ostream& operator<< (std::ostream& o, const quantity& q) {
    switch(q) {
        case quantity::real: return o << "real";
        case quantity::length: return o << "length";
        case quantity::mass: return o << "mass";
        case quantity::time: return o << "time";
        case quantity::current: return o << "current";
        case quantity::amount: return o << "amount";
        case quantity::temperature: return o << "temperature";
        case quantity::charge: return o << "charge";
        case quantity::frequency: return o << "frequency";
        case quantity::voltage: return o << "voltage";
        case quantity::resistance: return o << "resistance";
        case quantity::conductance: return o << "conductance";
        case quantity::capacitance: return o << "capacitance";
        case quantity::force: return o << "force";
        case quantity::energy: return o << "energy";
        case quantity::power: return o << "power";
        case quantity::area: return o << "area";
        case quantity::volume: return o << "volume";
        case quantity::concentration: return o << "concentration";
        default: return o;
    }
}

//integer_type;
std::ostream& operator<< (std::ostream& o, const integer_type& q) {
    return o << "(integer_type " << q.val << " " << q.loc << ")";
}

//quantity_type;
quantity_type::quantity_type(tok t, src_location loc): loc(loc) {
    if (auto q = gen_quantity(t)) {
        type = q.value();
    } else {
        throw std::runtime_error("Unexpected unary operator token");
    };
};
std::ostream& operator<< (std::ostream& o, const quantity_type& q) {
    return o << "(quantity_type " << q.type << " " << q.loc << ")";
}

//quantity_binary_type;
quantity_binary_type::quantity_binary_type(tok t, t_expr lhs, t_expr rhs, const src_location& loc):
    lhs(std::move(lhs)), rhs(std::move(rhs)), loc(loc) {
    if (auto bop = gen_binary_op(t)) {
        op = bop.value();
    } else {
        throw std::runtime_error("Unexpected binary operator token");
    };
}
std::ostream& operator<< (std::ostream& o, const quantity_binary_type& q) {
    o << "(quantity_binary_type " << q.op << " ";
    std::visit([&](auto&& c){o << c;}, *q.lhs);
    o << " ";
    std::visit([&](auto&& c){o << c;}, *q.rhs);
    return o << " " << q.loc << ")";
}

//boolean_type;
std::ostream& operator<< (std::ostream& o, const boolean_type& q) {
    return o << "(boolean_type " << q.loc << ")";
}

//record_type;
std::ostream& operator<< (std::ostream& o, const record_type& q) {
    o << "(record_type ";
    for (const auto& f: q.fields) {
        o << f.first << ":";
        std::visit([&](auto&& c) { o << c << " "; }, *(f.second));
    }
    return o << q.loc << ")";
}

//record_alias;
std::ostream& operator<< (std::ostream& o, const record_alias_type& q) {
    return o << "(record_alias_type " << q.name << " " << q.loc << ")";
}
} // namespace t_raw_ir
} // namespace al