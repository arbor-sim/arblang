#pragma once

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <arblang/token.hpp>
#include <arblang/visitor.hpp>

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
    conductance,
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

    integer_type(int val, const src_location& loc): val(val), loc(loc) {};
};

struct quantity_type {
    quantity type;
    src_location loc;

    quantity_type(quantity q, src_location loc): type(q), loc(std::move(loc)) {};
    quantity_type(tok t, src_location loc);
};

struct quantity_binary_type {
    t_binary_op op;
    t_expr lhs;
    t_expr rhs;
    src_location loc;

    quantity_binary_type(t_binary_op op, t_expr lhs, t_expr rhs, const src_location& loc):
        op(op), lhs(std::move(lhs)), rhs(std::move(rhs)), loc(loc) {};
    quantity_binary_type(tok t, t_expr lhs, t_expr rhs, const src_location& loc);
};

struct boolean_type {
    src_location loc;

    boolean_type(src_location loc): loc(loc) {};
};

struct record_type {
    std::vector<std::pair<std::string, t_expr>> fields;
    src_location loc;

    record_type(std::vector<std::pair<std::string, t_expr>> fields, src_location loc): fields(std::move(fields)), loc(loc) {};
};

struct record_alias_type {
    std::string name;
    src_location loc;

    record_alias_type(std::string name, src_location loc): name(std::move(name)), loc(loc) {};
};

std::string to_string(const t_binary_op&);
std::string to_string(const quantity&, int indent=0);
std::string to_string(const integer_type&, int indent=0);
std::string to_string(const quantity_type&, int indent=0);
std::string to_string(const quantity_binary_type&, int indent=0);
std::string to_string(const boolean_type&, int indent=0);
std::string to_string(const record_type&, int indent=0);
std::string to_string(const record_alias_type&, int indent=0);

template <typename T, typename... Args>
t_expr make_t_expr(Args&&... args) {
    return t_expr(new type_expr(T(std::forward<Args>(args)...)));
}

bool verify_type(const t_expr& u);
t_expr type_of(const bindable& b, const src_location& loc);
t_expr type_of(const affectable& a, const src_location& loc);

} // namespace t_raw_ir
} // namespace al