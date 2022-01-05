#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <unordered_map>

#include <arblang/unit_expressions.hpp>

namespace al {
namespace u_raw_ir {

std::unordered_map<std::string, unit_sym> string_to_sym = {
        {"m",   unit_sym::m},
        {"g",   unit_sym::g},
        {"s",   unit_sym::s},
        {"A",   unit_sym::A},
        {"K",   unit_sym::K},
        {"mol", unit_sym::mol},
        {"Hz",  unit_sym::Hz},
        {"L",   unit_sym::L},
        {"l",   unit_sym::l},
        {"N",   unit_sym::N},
        {"Pa",  unit_sym::Pa},
        {"W",   unit_sym::W},
        {"J",   unit_sym::J},
        {"C",   unit_sym::C},
        {"V",   unit_sym::V},
        {"F",   unit_sym::F},
        {"H",   unit_sym::H},
        {"Ohm", unit_sym::Ohm},
        {"S",   unit_sym::S},
        {"M",   unit_sym::M},
};

std::unordered_map<std::string, unit_pref> string_to_pref = {
        {"Y",  unit_pref::Y},
        {"Z",  unit_pref::Z},
        {"E",  unit_pref::E},
        {"P",  unit_pref::P},
        {"T",  unit_pref::T},
        {"G",  unit_pref::G},
        {"M",  unit_pref::M},
        {"k",  unit_pref::k},
        {"h",  unit_pref::h},
        {"da", unit_pref::da},
        {"d",  unit_pref::d},
        {"c",  unit_pref::c},
        {"m",  unit_pref::m},
        {"u",  unit_pref::u},
        {"n",  unit_pref::n},
        {"p",  unit_pref::p},
        {"f",  unit_pref::f},
        {"a",  unit_pref::a},
        {"z",  unit_pref::z},
        {"y",  unit_pref::y},
};

std::unordered_map<unit_sym, std::string> sym_to_string = {
        {unit_sym::m, "m"},
        {unit_sym::g, "g"},
        {unit_sym::s, "s"},
        {unit_sym::A, "A"},
        {unit_sym::K, "K"},
        {unit_sym::mol, "mol"},
        {unit_sym::Hz, "Hz"},
        {unit_sym::L, "L"},
        {unit_sym::l, "l"},
        {unit_sym::N, "N"},
        {unit_sym::Pa, "Pa"},
        {unit_sym::W, "W"},
        {unit_sym::J, "J"},
        {unit_sym::C, "C"},
        {unit_sym::V, "V"},
        {unit_sym::F, "F"},
        {unit_sym::H, "H"},
        {unit_sym::Ohm, "Ohm"},
        {unit_sym::S, "S"},
        {unit_sym::M, "M"},
};

std::unordered_map<unit_pref, std::string> pref_to_string = {
        {unit_pref::Y, "Y"},
        {unit_pref::Z, "Z"},
        {unit_pref::E, "E"},
        {unit_pref::P, "P"},
        {unit_pref::T, "T"},
        {unit_pref::G, "G"},
        {unit_pref::M, "M"},
        {unit_pref::k, "k"},
        {unit_pref::h, "h"},
        {unit_pref::da, "da"},
        {unit_pref::d, "d"},
        {unit_pref::c, "c"},
        {unit_pref::m, "m"},
        {unit_pref::u, "u"},
        {unit_pref::n, "n"},
        {unit_pref::p, "p"},
        {unit_pref::f, "f"},
        {unit_pref::a, "a"},
        {unit_pref::z, "z"},
        {unit_pref::y, "y"},
};

std::unordered_map<unit_pref, int> pref_to_factor = {
        {unit_pref::Y,  24},
        {unit_pref::Z,  21},
        {unit_pref::E,  18},
        {unit_pref::P,  15},
        {unit_pref::T,  12},
        {unit_pref::G,  9},
        {unit_pref::M,  6},
        {unit_pref::k,  3},
        {unit_pref::h,  2},
        {unit_pref::da, 1},
        {unit_pref::d,  -1},
        {unit_pref::c,  -2},
        {unit_pref::m,  -3},
        {unit_pref::u,  -6},
        {unit_pref::n,  -9},
        {unit_pref::p,  -12},
        {unit_pref::f,  -15},
        {unit_pref::a,  -18},
        {unit_pref::z,  -21},
        {unit_pref::y,  -24},
        {unit_pref::none, 0},
};

std::optional<unit> check_simple_unit(const std::string& s) {
    // If the string is a unit symbol, return with no prefix.
    if (string_to_sym.count(s)) return unit{unit_pref::none, string_to_sym.at(s)};

    // Otherwise, must contain a prefix or is not a unit.
    auto it = s.begin();
    std::string pref_str;

    while (*it) {
        pref_str += *it++;
        if (string_to_pref.count(pref_str)) {
            auto unit_str = std::string(it, s.end());
            if (string_to_sym.count(unit_str)) return unit{string_to_pref.at(pref_str), string_to_sym.at(unit_str)};
        }
    }
    return {};
}

std::optional<u_binary_op> gen_binary_op(tok t) {
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

std::string to_string(const u_binary_op& op) {
    switch(op) {
        case u_binary_op::mul: return "*";
        case u_binary_op::div: return "/";
        case u_binary_op::pow: return "^";
        default: return {};
    }
}

//quantity_binary_type;
binary_unit::binary_unit(tok t, u_expr l, u_expr r, const src_location& location): lhs(std::move(l)), rhs(std::move(r)), loc(location) {
    if (auto bop = gen_binary_op(t)) {
        op = bop.value();
    } else {
        throw std::runtime_error("Unexpected binary operator token");
    }
}

std::string to_string(const binary_unit& u, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(binary_unit " + to_string(u.op) + "\n";
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *u.lhs);
    std::visit([&](auto&& c){str += (to_string(c, indent+1) + "\n");}, *u.rhs);
    return str + double_indent + to_string(u.loc) + ")";
}
std::string to_string(const integer_unit& u, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(integer_unit\n";
    str += (double_indent + std::to_string(u.val) + "\n");
    return str + double_indent + to_string(u.loc) + ")";
}
std::string to_string(const simple_unit& u, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    return single_indent + "(simple_unit " + pref_to_string[u.val.prefix] + sym_to_string[u.val.symbol] + " " + to_string(u.loc) + ")";
}
std::string to_string(const no_unit& u, int indent) {
    return {};
}

t_raw_ir::t_binary_op to_t_binary_op(u_binary_op op) {
    switch (op) {
        case u_binary_op::mul: return t_raw_ir::t_binary_op::mul;
        case u_binary_op::div: return t_raw_ir::t_binary_op::div;
        case u_binary_op::pow: return t_raw_ir::t_binary_op::pow;
    }
    return {};
}

t_raw_ir::t_expr to_type(const binary_unit& u) {
    using namespace t_raw_ir;
    auto t_lhs = std::visit([&](auto&& c){return to_type(c);}, *u.lhs);
    auto t_rhs = std::visit([&](auto&& c){return to_type(c);}, *u.rhs);
    return make_t_expr<quantity_binary_type>(to_t_binary_op(u.op), t_lhs, t_rhs, u.loc);
}
t_raw_ir::t_expr to_type(const integer_unit& u) {
    using namespace t_raw_ir;
    return make_t_expr<integer_type>(u.val, u.loc);
}
t_raw_ir::t_expr to_type(const simple_unit& u) {
    using namespace t_raw_ir;
    switch (u.val.symbol) {
        case unit_sym::A:   return make_t_expr<quantity_type>(quantity::current, u.loc);
        case unit_sym::m:   return make_t_expr<quantity_type>(quantity::length, u.loc);
        case unit_sym::g:   return make_t_expr<quantity_type>(quantity::mass, u.loc);
        case unit_sym::s:   return make_t_expr<quantity_type>(quantity::time, u.loc);
        case unit_sym::K:   return make_t_expr<quantity_type>(quantity::temperature, u.loc);
        case unit_sym::mol: return make_t_expr<quantity_type>(quantity::amount, u.loc);
        case unit_sym::Hz:  return make_t_expr<quantity_type>(quantity::frequency, u.loc);
        case unit_sym::L:   return make_t_expr<quantity_type>(quantity::volume, u.loc);
        case unit_sym::l:   return make_t_expr<quantity_type>(quantity::volume, u.loc);
        case unit_sym::N:   return make_t_expr<quantity_type>(quantity::force, u.loc);
        case unit_sym::Pa:  return make_t_expr<quantity_type>(quantity::pressure, u.loc);
        case unit_sym::W:   return make_t_expr<quantity_type>(quantity::power, u.loc);
        case unit_sym::J:   return make_t_expr<quantity_type>(quantity::energy, u.loc);
        case unit_sym::C:   return make_t_expr<quantity_type>(quantity::charge, u.loc);
        case unit_sym::V:   return make_t_expr<quantity_type>(quantity::voltage, u.loc);
        case unit_sym::F:   return make_t_expr<quantity_type>(quantity::capacitance, u.loc);
        case unit_sym::H:   return make_t_expr<quantity_type>(quantity::inductance, u.loc);
        case unit_sym::Ohm: return make_t_expr<quantity_type>(quantity::resistance, u.loc);
        case unit_sym::S:   return make_t_expr<quantity_type>(quantity::conductance, u.loc);
        case unit_sym::M:   return make_t_expr<quantity_type>(quantity::concentration, u.loc);
    }
    return {};
}

std::pair<u_expr, int> normalize_unit(const binary_unit& u) {
    auto lhs = std::visit([&](auto&& c) {return normalize_unit(c);}, *u.lhs);
    auto rhs = std::visit([&](auto&& c) {return normalize_unit(c);}, *u.rhs);
    int factor;
    switch (u.op) {
        case u_binary_op::mul: factor = lhs.second + rhs.second; break;
        case u_binary_op::div: factor = lhs.second - rhs.second; break;
        case u_binary_op::pow: factor = lhs.second * rhs.second; break;
    }
    return {make_u_expr<binary_unit>(u.op, lhs.first, rhs.first, u.loc), factor};
}
std::pair<u_expr, int> normalize_unit(const integer_unit& u) {
    return {make_u_expr<integer_unit>(u), u.val};
}
std::pair<u_expr, int> normalize_unit(const simple_unit& u) {
    auto base_unit = u.val;
    auto factor = pref_to_factor[base_unit.prefix];
    base_unit.prefix = unit_pref::none;
    return {make_u_expr<simple_unit>(base_unit, u.loc), factor};
}
std::pair<u_expr, int> normalize_unit(const no_unit& u) {
    return {make_u_expr<no_unit>(), 1};
}

t_raw_ir::t_expr to_type(const no_unit& u) {
    using namespace t_raw_ir;
    return make_t_expr<quantity_type>(quantity::resistance, src_location{});
}

bool verify_sub_units(const u_expr& u) {
    auto is_int  = [](const u_expr& t) {return std::get_if<integer_unit>(t.get());};
    return std::visit(overloaded {
        [&](const simple_unit& t) {return true;},
        [&](const integer_unit& t) {return true;},
        [&](const no_unit& t) {return true;},
        [&](const binary_unit& t) {
            if (is_int(t.lhs)) return false;
            if ((t.op == u_binary_op::pow) && !is_int(t.rhs)) return false;
            if ((t.op != u_binary_op::pow) && is_int(t.rhs))  return false;
            return verify_sub_units(t.lhs) && verify_sub_units(t.rhs);
        }
    }, *u);
}

bool verify_unit(const u_expr& u) {
    return std::visit(overloaded {
        [&](const simple_unit& t) {return true;},
        [&](const no_unit& t) {return true;},
        [&](const binary_unit& t) {return verify_sub_units(u);},
        [&](const integer_unit& t) {return false;}
    }, *u);
}

} // namespace u_raw_ir
} // namespace al