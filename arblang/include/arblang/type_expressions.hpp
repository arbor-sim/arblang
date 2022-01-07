#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <arblang/token.hpp>
#include <arblang/common.hpp>
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
    length,       // default unit um
    mass,         // default unit g
    time,         // default unit ms
    current,      // default unit nA
    amount,       // default unit mol
    temperature,  // default unit K
    charge,       // default unit C
    frequency,    // default unit Hz
    voltage,      // default unit mV
    resistance,   // default unit Ohm
    conductance,  // default unit uS
    capacitance,  // default unit F
    inductance,   // default unit H
    force,        // default unit N
    pressure,     // default unit Pa
    energy,       // default unit J
    power,        // default unit W
    area,         // default unit um^2
    volume,       // default unit m^3
    concentration // default unit mmol/L
    // current_density (arbor) = A/m²
    // Point mechanism contributions are in [nA]; CV area in [µm²].
    // nA/µm² = 1000 A/m².
    // F = 1/area * [nA/µm²] / [A/m²] = 1000/area.
    // Density contributions are in mA/cm² = 10^-3/10^-4 A/m² = 10 A/m².
    // F = 10 [A/m²]/ [A/m²] = 10.
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

bool verify_type(const t_expr& u);

template <typename T, typename... Args>
t_expr make_t_expr(Args&&... args) {
    return t_expr(new type_expr(T(std::forward<Args>(args)...)));
}
} // namespace t_raw_ir
} // namespace al