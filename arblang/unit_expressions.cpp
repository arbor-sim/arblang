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

std::optional<unit> is_unit(const std::string& s) {
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

std::ostream& operator<< (std::ostream& o, const u_binary_op& op) {
    switch(op) {
        case u_binary_op::mul: return o << "*";
        case u_binary_op::div: return o << "/";
        case u_binary_op::pow: return o << "^";
        default: return o;
    }
}

//quantity_binary_type;
binary_unit::binary_unit(tok t, u_expr lhs, u_expr rhs, const src_location& loc): lhs(std::move(lhs)), rhs(std::move(rhs)), loc(loc) {
    if (auto bop = gen_binary_op(t)) {
        op = bop.value();
        if (op != u_binary_op::pow) {
            if (std::get_if<integer_unit>(lhs.get()) || std::get_if<integer_unit>(rhs.get())) {
                throw std::runtime_error("Invalid quantity expression");
            }
        } else {
            if (std::get_if<integer_unit>(lhs.get())) {
                throw std::runtime_error("Invalid quantity expression");
            }
        };
    } else {
        throw std::runtime_error("Unexpected binary operator token");
    };
}
std::ostream& operator<< (std::ostream& o, const binary_unit& u) {
    o << "(binary_unit " << u.op << " ";
    std::visit([&](auto&& c){o << c;}, *u.lhs);
    o << " ";
    std::visit([&](auto&& c){o << c;}, *u.rhs);
    return o << " " << u.loc << ")";
}

std::ostream& operator<< (std::ostream& o, const integer_unit& u) {
    return o << "(integer_unit " << u.val << " " << u.loc << ")";
}
std::ostream& operator<< (std::ostream& o, const simple_unit& u) {
    return o << "(simple_unit " << u.spelling << " " << u.loc << ")";
}
} // namespace u_raw_ir
} // namespace al