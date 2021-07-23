#pragma once

#include <string>
#include <vector>

namespace al {
namespace types {

struct integer_type;
struct quantity_type;
struct quantity_product_type;
struct quantity_quotient_type;
struct quantity_power_type;
struct boolean_type;
struct record_type;
struct record_alias;

using type_expr = std::variant<
    integer_type,
    quantity_type,
    quantity_product_type,
    quantity_quotient_type,
    quantity_power_type,
    boolean_type,
    record_type,
    record_alias>;

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
    integer_type(int val, src_location loc): val(val), loc(loc) {};
};

struct quantity_type {
    quantity type;
    src_location loc;

    quantity_type(quantity_type&&) = default;
    quantity_type(tok t, src_location loc): loc(loc) {
        if (auto q = gen_quantity(t)) {
            type = q.value();
        } else {
            throw std::runtime_error("Unexpected unary operator token");
        };
    };
};

struct quantity_product_type {
    t_expr lhs;
    t_expr rhs;
    src_location loc;

    quantity_product_type(quantity_product_type&&) = default;
    quantity_product_type(t_expr lhs, t_expr rhs, src_location loc): lhs(lhs), rhs(rhs), loc(loc) {};
};

struct quantity_quotient_type {
    t_expr lhs;
    t_expr rhs;
    src_location loc;

    quantity_quotient_type(quantity_quotient_type&&) = default;
    quantity_quotient_type(t_expr lhs, t_expr rhs, src_location loc): lhs(lhs), rhs(rhs), loc(loc) {};
};

struct quantity_power_type {
    t_expr lhs;
    int pow;
    src_location loc;

    quantity_power_type(quantity_power_type&&) = default;
    quantity_power_type(t_expr lhs, int pow, src_location loc): lhs(lhs), pow(pow), loc(loc) {};
};

struct boolean_type {};

struct record_type {
    std::vector<t_expr> fields;
    src_location loc;

    record_type(record_type&&) = default;
    record_type(std::vector<t_expr> fields, src_location loc): fields(std::move(fields)), loc(loc) {};
};

struct record_alias {
    std::string name;
    src_location loc;

    record_alias(record_alias&&) = default;
    record_alias(std::string name, src_location loc): name(std::move(name)), loc(loc) {};
};

template <typename T, typename... Args>
t_expr make_t_expr(Args&&... args) {
    return t_expr(new type_expr(T(std::forward<Args>(args)...)));
}

} // namespace types
} // namespace al