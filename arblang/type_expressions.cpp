#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <arblang/common.hpp>
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

std::string to_string(const t_binary_op& op) {
    switch(op) {
        case t_binary_op::mul: return "*";
        case t_binary_op::div: return "/";
        case t_binary_op::pow: return "^";
        default: return {};
    }

}

std::string to_string(const quantity& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    switch(q) {
        case quantity::real:          return single_indent + "real";
        case quantity::length:        return single_indent + "length";
        case quantity::mass:          return single_indent + "mass";
        case quantity::time:          return single_indent + "time";
        case quantity::current:       return single_indent + "current";
        case quantity::amount:        return single_indent + "amount";
        case quantity::temperature:   return single_indent + "temperature";
        case quantity::charge:        return single_indent + "charge";
        case quantity::frequency:     return single_indent + "frequency";
        case quantity::voltage:       return single_indent + "voltage";
        case quantity::resistance:    return single_indent + "resistance";
        case quantity::conductance:   return single_indent + "conductance";
        case quantity::capacitance:   return single_indent + "capacitance";
        case quantity::force:         return single_indent + "force";
        case quantity::energy:        return single_indent + "energy";
        case quantity::power:         return single_indent + "power";
        case quantity::area:          return single_indent + "area";
        case quantity::volume:        return single_indent + "volume";
        case quantity::concentration: return single_indent + "concentration";
        default: return single_indent;
    }
}

//integer_type;
std::string to_string(const integer_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent +  "(integer_type\n";
    str += (double_indent + std::to_string(q.val) + "\n");
    return str + double_indent + to_string(q.loc) + ")";
}

//quantity_type;
quantity_type::quantity_type(tok t, src_location loc): loc(loc) {
    if (auto q = gen_quantity(t)) {
        type = q.value();
    } else {
        throw std::runtime_error("Unexpected unary operator token");
    };
};
std::string to_string(const quantity_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(quantity_type\n";
    str += (double_indent + to_string(q.type) + "\n");
    return str + double_indent + to_string(q.loc) + ")";
}

//quantity_binary_type;
quantity_binary_type::quantity_binary_type(tok t, t_expr l, t_expr r, const src_location& location):
    lhs(std::move(l)), rhs(std::move(r)), loc(location) {
    auto verify = [](const auto&& t) {return std::get_if<integer_type>(t) || std::get_if<quantity_type>(t) ||std::get_if<quantity_binary_type>(t);};
    if(!verify(lhs.get()) || !verify(rhs.get())) {
        throw std::runtime_error("Invalid quantity expression");
    }
    if (auto bop = gen_binary_op(t)) {
        op = bop.value();
        if (op != t_binary_op::pow) {
            if (std::get_if<integer_type>(lhs.get()) || std::get_if<integer_type>(rhs.get())) {
                throw std::runtime_error("Invalid quantity expression");
            }
        } else {
            if (std::get_if<integer_type>(lhs.get()) || !std::get_if<integer_type>(rhs.get())) {
                throw std::runtime_error("Invalid quantity expression");
            }
        };
    } else {
        throw std::runtime_error("Unexpected binary operator token");
    };
}
std::string to_string(const quantity_binary_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(quantity_binary_type " + to_string(q.op) + "\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *q.lhs);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *q.rhs);
    return str + double_indent + to_string(q.loc) + ")";
}

//boolean_type;
std::string to_string(const boolean_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    return single_indent + "(boolean_type " + to_string(q.loc) + ")";
}

//record_type;
std::string to_string(const record_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent +  "(record_type\n";
    for (const auto& f: q.fields) {
        std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *(f.second));
    }
    return str + double_indent + to_string(q.loc) + ")";
}

//record_alias;
std::string to_string(const record_alias_type& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(record_alias_type\n";
    str += (double_indent + q.name + "\n");
    return str + double_indent + to_string(q.loc) + ")";
}

bool verify_sub_types(const t_expr& u) {
    auto is_int  = [](const t_expr& t) {return std::get_if<integer_type>(t.get());};
    return std::visit(overloaded {
            [&](const boolean_type& t)      {return false;},
            [&](const record_type& t)       {return false;},
            [&](const record_alias_type& t) {return false;},
            [&](const quantity_type& t)     {return true;},
            [&](const integer_type& t)      {return true;},
            [&](const quantity_binary_type& t) {
                if (is_int(t.lhs)) return false;
                if ((t.op == t_binary_op::pow) && !is_int(t.rhs)) return false;
                if ((t.op != t_binary_op::pow) && is_int(t.rhs))  return false;
                return verify_sub_types(t.lhs) && verify_sub_types(t.rhs);
            },
    }, *u);
}

bool verify_type(const t_expr& u) {
    return std::visit(overloaded {
        [&](const boolean_type& t)      {return true;},
        [&](const record_type& t)       {return true;},
        [&](const record_alias_type& t) {return true;},
        [&](const quantity_type& t)     {return true;},
        [&](const integer_type& t)      {return false;},
        [&](const quantity_binary_type& t) {return verify_sub_types(u);}
    }, *u);
}

t_expr type_of(const bindable& b, const src_location& loc) {
    switch (b) {
        case bindable::molar_flux: {
            auto lhs = make_t_expr<quantity_type>(quantity::amount, loc);
            auto rhs = make_t_expr<quantity_type>(quantity::area, loc);
            lhs = make_t_expr<quantity_binary_type>(t_binary_op::div, lhs, rhs, loc);
            rhs = make_t_expr<quantity_type>(quantity::time, loc);
            return make_t_expr<quantity_binary_type>(t_binary_op::div, lhs, rhs, loc);
        };
        case bindable::current_density: {
            auto lhs = make_t_expr<quantity_type>(quantity::current, loc);
            auto rhs = make_t_expr<quantity_type>(quantity::area, loc);
            return make_t_expr<quantity_binary_type>(t_binary_op::div, lhs, rhs, loc);
        }
        case bindable::charge:                 return make_t_expr<quantity_type>(quantity::real, loc);
        case bindable::external_concentration:
        case bindable::internal_concentration: return make_t_expr<quantity_type>(quantity::concentration, loc);
        case bindable::membrane_potential:
        case bindable::nernst_potential:       return make_t_expr<quantity_type>(quantity::voltage, loc);
        case bindable::temperature:            return make_t_expr<quantity_type>(quantity::temperature, loc);
        default: return {};
    }
}

t_expr type_of(const affectable& a, const src_location& loc) {
    switch (a) {
        case affectable::molar_flux: {
            auto lhs = make_t_expr<quantity_type>(quantity::amount, loc);
            auto rhs = make_t_expr<quantity_type>(quantity::area, loc);
            lhs = make_t_expr<quantity_binary_type>(t_binary_op::div, lhs, rhs, loc);
            rhs = make_t_expr<quantity_type>(quantity::time, loc);
            return make_t_expr<quantity_binary_type>(t_binary_op::div, lhs, rhs, loc);
        };
        case affectable::molar_flow_rate: {
            auto lhs = make_t_expr<quantity_type>(quantity::amount, loc);
            auto rhs = make_t_expr<quantity_type>(quantity::time, loc);
            return make_t_expr<quantity_binary_type>(t_binary_op::div, lhs, rhs, loc);
        }
        case affectable::current_density: {
            auto lhs = make_t_expr<quantity_type>(quantity::current, loc);
            auto rhs = make_t_expr<quantity_type>(quantity::area, loc);
            return make_t_expr<quantity_binary_type>(t_binary_op::div, lhs, rhs, loc);
        }
        case affectable::current: {
            return make_t_expr<quantity_type>(quantity::current, loc);
        }
        case affectable::external_concentration_rate:
        case affectable::internal_concentration_rate: {
            auto lhs = make_t_expr<quantity_type>(quantity::concentration, loc);
            auto rhs = make_t_expr<quantity_type>(quantity::time, loc);
            return make_t_expr<quantity_binary_type>(t_binary_op::div, lhs, rhs, loc);
        }
        default: return {};
    }
}

} // namespace t_raw_ir
} // namespace al