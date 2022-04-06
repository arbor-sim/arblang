#include <optional>
#include <string>
#include <variant>
#include <unordered_map>

#include <fmt/core.h>

#include <arblang/parser/parsed_units.hpp>

namespace al {
namespace parsed_unit_ir {

std::optional<unit_sym> to_unit_symbol(const std::string& s) {
    if (s == "m")   return unit_sym::m;
    if (s == "g")   return unit_sym::g;
    if (s == "s")   return unit_sym::s;
    if (s == "A")   return unit_sym::A;
    if (s == "K")   return unit_sym::K;
    if (s == "mol") return unit_sym::mol;
    if (s == "Hz")  return unit_sym::Hz;
    if (s == "L")   return unit_sym::L;
    if (s == "l")   return unit_sym::l;
    if (s == "N")   return unit_sym::N;
    if (s == "Pa")  return unit_sym::Pa;
    if (s == "W")   return unit_sym::W;
    if (s == "J")   return unit_sym::J;
    if (s == "C")   return unit_sym::C;
    if (s == "V")   return unit_sym::V;
    if (s == "F")   return unit_sym::F;
    if (s == "H")   return unit_sym::H;
    if (s == "Ohm") return unit_sym::Ohm;
    if (s == "S")   return unit_sym::S;
    if (s == "M")   return unit_sym::M;
    return {};
};

std::optional<unit_pref> to_unit_prefix(const std::string& s) {
    if (s == "Y")  return unit_pref::Y;
    if (s == "Z")  return unit_pref::Z;
    if (s == "E")  return unit_pref::E;
    if (s == "P")  return unit_pref::P;
    if (s == "T")  return unit_pref::T;
    if (s == "G")  return unit_pref::G;
    if (s == "M")  return unit_pref::M;
    if (s == "k")  return unit_pref::k;
    if (s == "h")  return unit_pref::h;
    if (s == "da") return unit_pref::da;
    if (s == "d")  return unit_pref::d;
    if (s == "c")  return unit_pref::c;
    if (s == "m")  return unit_pref::m;
    if (s == "u")  return unit_pref::u;
    if (s == "n")  return unit_pref::n;
    if (s == "p")  return unit_pref::p;
    if (s == "f")  return unit_pref::f;
    if (s == "a")  return unit_pref::a;
    if (s == "z")  return unit_pref::z;
    if (s == "y")  return unit_pref::y;
    return {};
};

std::optional<int> to_prefix_factor(unit_pref p) {
    switch (p) {
        case unit_pref::Y:  return 24;
        case unit_pref::Z:  return 21;
        case unit_pref::E:  return 18;
        case unit_pref::P:  return 15;
        case unit_pref::T:  return 12;
        case unit_pref::G:  return 9;
        case unit_pref::M:  return 6;
        case unit_pref::k:  return 3;
        case unit_pref::h:  return 2;
        case unit_pref::da: return 1;
        case unit_pref::d:  return -1;
        case unit_pref::c:  return -2;
        case unit_pref::m:  return -3;
        case unit_pref::u:  return -6;
        case unit_pref::n:  return -9;
        case unit_pref::p:  return -12;
        case unit_pref::f:  return -15;
        case unit_pref::a:  return -18;
        case unit_pref::z:  return -21;
        case unit_pref::y:  return -24;
        case unit_pref::none: return 0;
        default: return {};
    }
    return {};
};

std::optional<u_binary_op> to_binary_op(tok t) {
    switch (t) {
        case tok::times:
            return u_binary_op::mul;
        case tok::divide:
            return u_binary_op::div;
        case tok::pow:
            return u_binary_op::pow;
        default: return {};
    }
}

//parsed_binary_unit
parsed_binary_unit::parsed_binary_unit(u_binary_op op, p_unit l, p_unit r, const src_location& location):
    op(op), lhs(std::move(l)), rhs(std::move(r)), loc(location)
{
    if (!verify()) {
        throw std::runtime_error(fmt::format("Invalid quantity expression at {}", to_string(location)));
    }
}

parsed_binary_unit::parsed_binary_unit(tok t, p_unit l, p_unit r, const src_location& location):
    lhs(std::move(l)), rhs(std::move(r)), loc(location)
{
    if (auto b_op = to_binary_op(t)) {
        op = b_op.value();
    } else {
        throw std::runtime_error("Unexpected binary operator token");
    }
    if (!verify()) {
        throw std::runtime_error(fmt::format("Invalid quantity expression at {}", to_string(location)));
    }
}


bool parsed_binary_unit::verify() const {
    auto pow_op = (op == u_binary_op::pow);

    if (is_parsed_integer_unit(lhs)) return false;
    if ((pow_op && !is_parsed_integer_unit(rhs)) || (!pow_op && is_parsed_integer_unit(rhs))) return false;

    auto is_allowed = [](const p_unit& u) {
        return std::visit(al::util::overloaded {
                [&](const parsed_simple_unit& t)  {return true;},
                [&](const parsed_integer_unit& t) {return true;},
                [&](const parsed_no_unit& t)      {return false;},
                [&](const parsed_binary_unit& t)  {return true;},
        }, *u);
    };
    if (!is_allowed(lhs) || !is_allowed(rhs)) return false;
    return true;
}

// check_parsed_simple_unit
// Optionally return a "simple unit" (i.e. a an optional prefix + unit symbol)
std::optional<unit> check_parsed_simple_unit(const std::string& s) {
    // If the string is a unit symbol, return with no prefix.
    if (auto sym = to_unit_symbol(s)) return unit{unit_pref::none, sym.value()};

    // Otherwise, must contain a prefix or is not a unit.
    auto it = s.begin();
    std::string pref_str;

    while (*it) {
        pref_str += *it++;
        if (auto pref = to_unit_prefix(pref_str)) {
            auto unit_str = std::string(it, s.end());
            if (auto sym = to_unit_symbol(unit_str)) return unit{pref.value(), sym.value()};
        }
    }
    return {};
}

// to_type
parsed_type_ir::p_type to_type(const parsed_binary_unit& u) {
    using namespace parsed_type_ir;
    t_binary_op op;
    switch (u.op) {
        case u_binary_op::mul: op = parsed_type_ir::t_binary_op::mul; break;
        case u_binary_op::div: op = parsed_type_ir::t_binary_op::div; break;
        case u_binary_op::pow: op = parsed_type_ir::t_binary_op::pow; break;
    }
    return make_ptype<parsed_binary_quantity_type>(op, to_type(u.lhs), to_type(u.rhs), u.loc);
}

parsed_type_ir::p_type to_type(const parsed_integer_unit& u) {
    using namespace parsed_type_ir;
    return make_ptype<parsed_integer_type>(u.val, u.loc);
}

parsed_type_ir::p_type to_type(const parsed_simple_unit& u) {
    using namespace parsed_type_ir;
    switch (u.val.symbol) {
        case unit_sym::A:   return make_ptype<parsed_quantity_type>(quantity::current, u.loc);
        case unit_sym::m:   return make_ptype<parsed_quantity_type>(quantity::length, u.loc);
        case unit_sym::g:   return make_ptype<parsed_quantity_type>(quantity::mass, u.loc);
        case unit_sym::s:   return make_ptype<parsed_quantity_type>(quantity::time, u.loc);
        case unit_sym::K:   return make_ptype<parsed_quantity_type>(quantity::temperature, u.loc);
        case unit_sym::mol: return make_ptype<parsed_quantity_type>(quantity::amount, u.loc);
        case unit_sym::Hz:  return make_ptype<parsed_quantity_type>(quantity::frequency, u.loc);
        case unit_sym::L:   return make_ptype<parsed_quantity_type>(quantity::volume, u.loc);
        case unit_sym::l:   return make_ptype<parsed_quantity_type>(quantity::volume, u.loc);
        case unit_sym::N:   return make_ptype<parsed_quantity_type>(quantity::force, u.loc);
        case unit_sym::Pa:  return make_ptype<parsed_quantity_type>(quantity::pressure, u.loc);
        case unit_sym::W:   return make_ptype<parsed_quantity_type>(quantity::power, u.loc);
        case unit_sym::J:   return make_ptype<parsed_quantity_type>(quantity::energy, u.loc);
        case unit_sym::C:   return make_ptype<parsed_quantity_type>(quantity::charge, u.loc);
        case unit_sym::V:   return make_ptype<parsed_quantity_type>(quantity::voltage, u.loc);
        case unit_sym::F:   return make_ptype<parsed_quantity_type>(quantity::capacitance, u.loc);
        case unit_sym::H:   return make_ptype<parsed_quantity_type>(quantity::inductance, u.loc);
        case unit_sym::Ohm: return make_ptype<parsed_quantity_type>(quantity::resistance, u.loc);
        case unit_sym::S:   return make_ptype<parsed_quantity_type>(quantity::conductance, u.loc);
        case unit_sym::M:   return make_ptype<parsed_quantity_type>(quantity::concentration, u.loc);
    }
    return {};
}

parsed_type_ir::p_type to_type(const parsed_no_unit& u) {
    using namespace parsed_type_ir;
    return make_ptype<parsed_quantity_type>(quantity::real, src_location{});
}

parsed_type_ir::p_type to_type(const p_unit& u) {
    return std::visit([&](auto&& c){return to_type(c);}, *u);
}

// normalize
std::pair<p_unit, int> normalize_unit(const parsed_binary_unit& u) {
    auto lhs = normalize_unit(u.lhs);
    auto rhs = normalize_unit(u.rhs);
    int factor;
    switch (u.op) {
        case u_binary_op::mul: factor = lhs.second + rhs.second; break;
        case u_binary_op::div: factor = lhs.second - rhs.second; break;
        case u_binary_op::pow: factor = lhs.second * rhs.second; break;
    }
    return {make_punit<parsed_binary_unit>(u.op, lhs.first, rhs.first, u.loc), factor};
}

std::pair<p_unit, int> normalize_unit(const parsed_integer_unit& u) {
    return {make_punit<parsed_integer_unit>(u), u.val};
}

std::pair<p_unit, int> normalize_unit(const parsed_simple_unit& u) {
    auto base_unit = u.val;
    auto factor = to_prefix_factor(base_unit.prefix).value();
    base_unit.prefix = unit_pref::none;
    return {make_punit<parsed_simple_unit>(base_unit, u.loc), factor};
}

std::pair<p_unit, int> normalize_unit(const parsed_no_unit& u) {
    return {make_punit<parsed_no_unit>(), 0};
}

std::pair<p_unit, int> normalize_unit(const p_unit& u) {
    return std::visit([&](auto&& c){return normalize_unit(c);}, *u);
}

// to_string
std::string to_string(unit_sym s) {
    switch (s) {
        case unit_sym::m:   return "m";
        case unit_sym::g:   return "g";
        case unit_sym::s:   return "s";
        case unit_sym::A:   return "A";
        case unit_sym::K:   return "K";
        case unit_sym::mol: return "mol";
        case unit_sym::Hz:  return "Hz";
        case unit_sym::L:   return "L";
        case unit_sym::l:   return "l";
        case unit_sym::N:   return "N";
        case unit_sym::Pa:  return "Pa";
        case unit_sym::W:   return "W";
        case unit_sym::J:   return "J";
        case unit_sym::C:   return "C";
        case unit_sym::V:   return "V";
        case unit_sym::F:   return "F";
        case unit_sym::H:   return "H";
        case unit_sym::Ohm: return "Ohm";
        case unit_sym::S:   return "S";
        case unit_sym::M:   return "M";
        default: return {};
    }
};

std::string to_string(unit_pref p) {
    switch (p) {
        case unit_pref::Y:  return "Y";
        case unit_pref::Z:  return "Z";
        case unit_pref::E:  return "E";
        case unit_pref::P:  return "P";
        case unit_pref::T:  return "T";
        case unit_pref::G:  return "G";
        case unit_pref::M:  return "M";
        case unit_pref::k:  return "k";
        case unit_pref::h:  return "h";
        case unit_pref::da: return "da";
        case unit_pref::d:  return "d";
        case unit_pref::c:  return "c";
        case unit_pref::m:  return "m";
        case unit_pref::u:  return "u";
        case unit_pref::n:  return "n";
        case unit_pref::p:  return "p";
        case unit_pref::f:  return "f";
        case unit_pref::a:  return "a";
        case unit_pref::z:  return "z";
        case unit_pref::y:  return "y";
        default: return {};
    }
};

std::string to_string(const u_binary_op& op) {
    switch(op) {
        case u_binary_op::mul: return "*";
        case u_binary_op::div: return "/";
        case u_binary_op::pow: return "^";
        default: return {};
    }
}

std::string to_string(const parsed_binary_unit& u, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_binary_unit " + to_string(u.op) + "\n";
    str += to_string(u.lhs, indent+1) + "\n";
    str += to_string(u.rhs, indent+1) + "\n";
    str += double_indent + to_string(u.loc) + ")";
    return str;
}

std::string to_string(const parsed_integer_unit& u, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(parsed_integer_unit\n";
    str += (double_indent + std::to_string(u.val) + "\n");
    str += double_indent + to_string(u.loc) + ")";
    return str;
}

std::string to_string(const parsed_simple_unit& u, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    return single_indent + "(parsed_simple_unit " + to_string(u.val.prefix) + to_string(u.val.symbol) + " " + to_string(u.loc) + ")";
}

std::string to_string(const parsed_no_unit& u, int indent) {
    return {};
}

std::string to_string(const p_unit& u , int indent) {
    return std::visit([&](auto&& c){return to_string(c, indent);}, *u);
}

std::optional<parsed_integer_unit> is_parsed_integer_unit(const p_unit& p) {
    if (!std::holds_alternative<parsed_integer_unit>(*p)) return {};
    return std::get<parsed_integer_unit>(*p);
}
std::optional<parsed_simple_unit> is_parsed_simple_unit(const p_unit& p) {
    if (!std::holds_alternative<parsed_simple_unit>(*p)) return {};
    return std::get<parsed_simple_unit>(*p);
}
std::optional<parsed_binary_unit> is_parsed_binary_unit(const p_unit& p) {
    if (!std::holds_alternative<parsed_binary_unit>(*p)) return {};
    return std::get<parsed_binary_unit>(*p);
}
std::optional<parsed_no_unit> is_parsed_no_unit(const p_unit& p) {
    if (!std::holds_alternative<parsed_no_unit>(*p)) return {};
    return std::get<parsed_no_unit>(*p);
}


} // namespace parsed_unit_ir
} // namespace al