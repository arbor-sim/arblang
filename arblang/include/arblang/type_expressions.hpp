#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <arblang/token.hpp>

namespace al {
namespace t_raw_ir {

struct integer_type;
struct quantity_type;
struct quantity_binary_type;
struct boolean_type;
struct record_type;
struct record_alias_type;

using type_expr = std::variant<
    integer_type,
    quantity_type,
    quantity_binary_type,
    boolean_type,
    record_type,
    record_alias_type>;

using t_expr = std::shared_ptr<type_expr>;

enum class quantity {
    real,
    length,
    mass,
    time,
    current,
    amount,
    temperature,
    charge,
    frequency,
    voltage,
    resistance,
    capacitance,
    force,
    energy,
    power,
    area,
    volume,
    concentration
};

enum class t_binary_op {
    mul, div, pow,
};

std::optional<quantity> gen_quantity(tok t) {
    switch (t) {
        case tok::real:
            return quantity::real;
        case tok::length:
            return quantity::length;
        case tok::mass:
            return quantity::mass;
        case tok::time:
            return quantity::time;
        case tok::current:
            return quantity::current;
        case tok::amount:
            return quantity::amount;
        case tok::temperature:
            return quantity::temperature;
        case tok::charge:
            return quantity::charge;
        case tok::frequency:
            return quantity::frequency;
        case tok::voltage:
            return quantity::voltage;
        case tok::resistance:
            return quantity::resistance;
        case tok::capacitance:
            return quantity::capacitance;
        case tok::force:
            return quantity::force;
        case tok::energy:
            return quantity::energy;
        case tok::power:
            return quantity::power;
        case tok::area:
            return quantity::area;
        case tok::volume:
            return quantity::volume;
        case tok::concentration:
            return quantity::concentration;
        default: return {};
    }
}

struct integer_type {
    int val;
    src_location loc;

    integer_type(integer_type&&) = default;
    integer_type(int val, const src_location& loc): val(val), loc(loc) {};
};

struct quantity_type {
    quantity type;
    src_location loc;

    quantity_type(quantity_type&&) = default;
    quantity_type(tok t, src_location loc);
};

struct quantity_binary_type {
    t_binary_op op;
    t_expr lhs;
    t_expr rhs;
    src_location loc;

    quantity_binary_type(quantity_binary_type&&) = default;
    quantity_binary_type(tok t, t_expr lhs, t_expr rhs, const src_location& loc);
};

struct boolean_type {
    src_location loc;

    boolean_type(boolean_type&&) = default;
    boolean_type(src_location loc): loc(loc) {};
};

struct record_type {
    std::vector<t_expr> fields;
    src_location loc;

    record_type(record_type&&) = default;
    record_type(std::vector<t_expr> fields, src_location loc): fields(std::move(fields)), loc(loc) {};
};

struct record_alias_type {
    std::string name;
    src_location loc;

    record_alias_type(record_alias_type&&) = default;
    record_alias_type(std::string name, src_location loc): name(std::move(name)), loc(loc) {};
};

std::ostream& operator<< (std::ostream&, const t_binary_op&);
std::ostream& operator<< (std::ostream&, const quantity&);
std::ostream& operator<< (std::ostream&, const integer_type&);
std::ostream& operator<< (std::ostream&, const quantity_type&);
std::ostream& operator<< (std::ostream&, const quantity_binary_type&);
std::ostream& operator<< (std::ostream&, const boolean_type&);
std::ostream& operator<< (std::ostream&, const record_type&);
std::ostream& operator<< (std::ostream&, const record_alias_type&);

template <typename T, typename... Args>
t_expr make_t_expr(Args&&... args) {
    return t_expr(new type_expr(T(std::forward<Args>(args)...)));
}

} // namespace t_raw_ir
} // namespace al