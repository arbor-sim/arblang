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

std::optional<quantity> gen_quantity(tok t);

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