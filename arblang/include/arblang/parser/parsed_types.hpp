#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <arblang/parser/token.hpp>
#include <arblang/util/common.hpp>
#include <arblang/util/visitor.hpp>

namespace al {
namespace parsed_type_ir {

struct parsed_integer_type;
struct parsed_quantity_type;
struct parsed_binary_quantity_type;
struct parsed_bool_type;
struct parsed_record_type;
struct parsed_record_alias_type;

using type_expr = std::variant<
    parsed_integer_type,
    parsed_quantity_type,
    parsed_binary_quantity_type,
    parsed_bool_type,
    parsed_record_type,
    parsed_record_alias_type>;

using p_type = std::shared_ptr<type_expr>;

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

struct parsed_integer_type {
    int val;
    src_location loc;

    parsed_integer_type(int val, const src_location& loc): val(val), loc(loc) {};
};

struct parsed_quantity_type {
    quantity type;
    src_location loc;

    parsed_quantity_type(quantity q, src_location loc): type(q), loc(std::move(loc)) {};
    parsed_quantity_type(tok t, src_location loc);
};

struct parsed_binary_quantity_type {
    t_binary_op op;
    p_type lhs;
    p_type rhs;
    src_location loc;

    parsed_binary_quantity_type(t_binary_op op, p_type lhs, p_type rhs, const src_location& loc);
    parsed_binary_quantity_type(tok t, p_type lhs, p_type rhs, const src_location& loc);

private:
    bool verify() const;
};

struct parsed_bool_type {
    src_location loc;

    parsed_bool_type(src_location loc): loc(loc) {};
};

struct parsed_record_type {
    std::vector<std::pair<std::string, p_type>> fields;
    src_location loc;

    parsed_record_type(std::vector<std::pair<std::string, p_type>> fields, src_location loc): fields(std::move(fields)), loc(loc) {};
};

struct parsed_record_alias_type {
    std::string name;
    src_location loc;

    parsed_record_alias_type(std::string name, src_location loc): name(std::move(name)), loc(loc) {};
};

// Generate string representation of type expression
std::string to_string(quantity);
std::string to_string(const p_type&, int indent = 0);

template <typename T, typename... Args>
p_type make_ptype(Args&&... args) {
    return p_type(new type_expr(T(std::forward<Args>(args)...)));
}
} // namespace parsed_type_ir
} // namespace al