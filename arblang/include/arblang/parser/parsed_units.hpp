#pragma once

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <variant>

#include <arblang/parser/token.hpp>
#include <arblang/parser/parsed_types.hpp>
#include <arblang/util/visitor.hpp>

namespace al {
namespace parsed_unit_ir {

struct parsed_integer_unit;
struct parsed_simple_unit;
struct parsed_binary_unit;
struct parsed_no_unit;

using parsed_unit = std::variant<
        parsed_integer_unit,
        parsed_simple_unit,
        parsed_binary_unit,
        parsed_no_unit>;

using p_unit = std::shared_ptr<parsed_unit>;

enum class unit_sym {
    m,   // meter;   length
    g,   // gram;    mass
    s,   // second;  time
    A,   // ampere;  current
    K,   // kelvin;  temperature
    mol, // mole;    amount
    Hz,  // herz;    frequency
    L,   // litre;   volume
    l,   // litre;   volume
    N,   // newton;  force
    Pa,  // pascal;  pressure
    W,   // watt;    power
    J,   // joule;   energy
    C,   // coulomb; charge
    V,   // volt;    voltage
    F,   // farad;   capacitance
    H,   // henry;   inductance
    Ohm, // ohm;     resistance
    S,   // siemens; conductance
    M,   // molar;   molarity
};

enum class unit_pref {
    Y,  // yotta;  10^24
    Z,  // zetta;  10^21
    E,  // exa;    10^18
    P,  // peta;   10^15
    T,  // tera;   10^12
    G,  // gega;   10^9
    M,  // mega;   10^6
    k,  // kilo;   10^3
    h,  // hector; 10^2
    da, // deca;   10^1
    d,  // deci;   10^-1
    c,  // centi;  10^-2
    m,  // milli;  10^-3
    u,  // micro;  10^-6
    n,  // nano;   10^-9
    p,  // pico;   10^-12
    f,  // femto;  10^-15
    a,  // atto;   10^-18
    z,  // zepto;  10^-21
    y,  // yocto;  10^-24
    none, // none; 10^0
};

struct unit {
    unit_pref prefix;
    unit_sym  symbol;
};

enum class u_binary_op {
    mul, div, pow,
};

struct parsed_integer_unit {
    int val;
    src_location loc;
    parsed_integer_unit(int val, const src_location& loc): val(val), loc(loc) {};
};

struct parsed_simple_unit {
    unit val;
    src_location loc;
    parsed_simple_unit(unit val, const src_location& loc): val(std::move(val)), loc(loc) {};
};

struct parsed_binary_unit {
    u_binary_op op;
    p_unit lhs;
    p_unit rhs;
    src_location loc;

    parsed_binary_unit(tok t, p_unit lhs, p_unit rhs, const src_location& loc);
    parsed_binary_unit(u_binary_op op, p_unit lhs, p_unit rhs, const src_location& loc);
private:
    bool verify() const;
};

struct parsed_no_unit{};

// Checks if a string is a simple unit
std::optional<unit> check_parsed_simple_unit(const std::string& s);

// Generate string representation of unit expression
std::string to_string(const p_unit&, int indent=0);

// Generate the equivalent type expression of a unit expression
parsed_type_ir::p_type to_type(const p_unit &);

// Normalize the unit into the base units and factor
// example: mV    -> {V, -3}
//          mV/mA -> {V/A, 0}
std::pair<p_unit, int> normalize_unit(const p_unit&);

template <typename T, typename... Args>
p_unit make_punit(Args&&... args) {
    return p_unit(new parsed_unit(T(std::forward<Args>(args)...)));
}

} // namespace parsed_unit_ir
} // namespace al