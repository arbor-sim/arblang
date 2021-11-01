#pragma once

#include <optional>
#include <string>

namespace al {
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

std::optional<unit> is_unit(const std::string& s);

} // namespace al