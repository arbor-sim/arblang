#include <stdexcept>

#include <arblang/type_expressions.hpp>
#include <arblang/resolved_types.hpp>

#define FMT_HEADER_ONLY YES
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/compile.h>

namespace al {
namespace t_resolved_ir {
using namespace t_raw_ir;
static std::unordered_map<quantity, normalized_type> normal_quantities = {
        //                                         m  g  s  A mol K
        {quantity::real,          normalized_type({ 0, 0, 0, 0, 0, 0})},
        {quantity::length,        normalized_type({ 1, 0, 0, 0, 0, 0})},
        {quantity::mass,          normalized_type({ 0, 1, 0, 0, 0, 0})},
        {quantity::time,          normalized_type({ 0, 0, 1, 0, 0, 0})},
        {quantity::current,       normalized_type({ 0, 0, 0, 1, 0, 0})},
        {quantity::amount,        normalized_type({ 0, 0, 0, 0, 1, 0})},
        {quantity::temperature,   normalized_type({ 0, 0, 0, 0, 0, 1})},
        {quantity::charge,        normalized_type({ 0, 0, 1, 1, 0, 0})},
        {quantity::frequency,     normalized_type({ 0, 0,-1, 0, 0, 0})},
        {quantity::voltage,       normalized_type({ 2, 1,-3,-1, 0, 0})},
        {quantity::resistance,    normalized_type({ 2, 1,-3,-2, 0, 0})},
        {quantity::conductance,   normalized_type({-2,-1, 3, 2, 0, 0})},
        {quantity::capacitance,   normalized_type({-2,-1, 4, 2, 0, 0})},
        {quantity::inductance,    normalized_type({ 2, 1,-2,-2, 0, 0})},
        {quantity::force,         normalized_type({ 1, 1,-2, 0, 0, 0})},
        {quantity::pressure,      normalized_type({-1, 1,-2, 0, 0, 0})},
        {quantity::energy,        normalized_type({ 2, 1,-2, 0, 0, 0})},
        {quantity::power,         normalized_type({ 2, 1,-3, 0, 0, 0})},
        {quantity::area,          normalized_type({ 2, 0, 0, 0, 0, 0})},
        {quantity::volume,        normalized_type({ 3, 0, 0, 0, 0, 0})},
        {quantity::concentration, normalized_type({-3, 0, 0, 0, 1, 0})},
};

std::unordered_map<quantity, int> normalized_type::q_map = {
        {quantity::length,         0},
        {quantity::mass,           1},
        {quantity::time,           2},
        {quantity::current,        3},
        {quantity::amount,         4},
        {quantity::temperature,    5},
};
bool normalized_type::is_real() {
    return std::all_of(q_pow.begin(), q_pow.end(), [](int i){return i==0;});
}
normalized_type& normalized_type::set(quantity q, int val) {
    if (!q_map.count(q)) {
        throw std::runtime_error("Internal compiler error: expected base SI quantity");
    }
    q_pow[q_map[q]] = val;
    return *this;
}

bool operator==(const normalized_type& lhs, const normalized_type& rhs) {
    return lhs.q_pow == rhs.q_pow;
}
bool operator!=(const normalized_type& lhs, const normalized_type& rhs) {
    return !(lhs.q_pow == rhs.q_pow);
}
normalized_type operator*(normalized_type& lhs, normalized_type& rhs) {
    normalized_type t = lhs;
    for (unsigned i = 0; i < 6; ++i) {
        t.q_pow[i] += rhs.q_pow[i];
    }
    return t;
}
normalized_type operator/(normalized_type& lhs, normalized_type& rhs) {
    normalized_type t = lhs;
    for (unsigned i = 0; i < 6; ++i) {
        t.q_pow[i] -= rhs.q_pow[i];
    }
    return t;
}
normalized_type operator^(normalized_type& lhs, int rhs) {
    normalized_type t = lhs;
    for (unsigned i = 0; i < 6; ++i) {
        t.q_pow[i] *= rhs;
    }
    return t;
}

// Resolve types
r_type resolve_type_of(const bindable& b, const src_location& loc) {
    normalized_type t;
    switch (b) {
        case bindable::molar_flux:             t.set(quantity::amount, 1).set(quantity::length, -2).set(quantity::time, -1); break;
        case bindable::current_density:        t.set(quantity::current, 1).set(quantity::length, -2); break;
        case bindable::charge:                 t.set(quantity::current, 1).set(quantity::time, 1); break;
        case bindable::external_concentration:
        case bindable::internal_concentration: t.set(quantity::amount, 1).set(quantity::length, -3); break;
        case bindable::temperature:            t.set(quantity::temperature, 1); break;
        case bindable::membrane_potential:
        case bindable::nernst_potential: {
            t.set(quantity::mass, 1);
            t.set(quantity::length, 2);
            t.set(quantity::time, -3);
            t.set(quantity::current, -1);
            break;
        }
        default: break;
    }
    return make_rtype<resolved_quantity>(t, loc);
}

r_type resolve_type_of(const affectable& a, const src_location& loc) {
    normalized_type t;
    switch (a) {
        case affectable::molar_flux:                  t.set(quantity::amount, 1).set(quantity::length, -2).set(quantity::time, -1); break;
        case affectable::molar_flow_rate:             t.set(quantity::amount, 1).set(quantity::time, -1); break;
        case affectable::current_density:             t.set(quantity::current, 1).set(quantity::length, -2); break;
        case affectable::current:                     t.set(quantity::current, 1); break;
        case affectable::external_concentration_rate:
        case affectable::internal_concentration_rate: t.set(quantity::amount, 1).set(quantity::length, -3).set(quantity::time, -1); break;
        default: break;
    }
    return make_rtype<resolved_quantity>(t, loc);
}

r_type resolve_type_of(const quantity_type& t, const std::unordered_map<std::string, r_type>&) {
    return make_rtype<resolved_quantity>(normal_quantities[t.type], t.loc);
}
r_type resolve_type_of(const quantity_binary_type& t, const std::unordered_map<std::string, r_type>& rec_alias) {
    auto nlhs = std::visit([&](auto&& c){return resolve_type_of(c, rec_alias);}, *t.lhs);
    if (!std::get_if<resolved_quantity>(nlhs.get())) {
        throw std::runtime_error(fmt::format("Internal compiler error: expected resolved quantity type at lhs of {}",
                                             to_string(t.loc)));
    }
    auto nlhs_q = std::get<resolved_quantity>(*nlhs);

    normalized_type type;
    if (t.op == t_binary_op::pow) {
        if (!std::get_if<integer_type>(t.rhs.get())) {
            throw std::runtime_error(fmt::format("Internal compiler error: expected integer type at rhs of {}",
                                                 to_string(t.loc)));
        }
        auto nrhs_q = std::get<integer_type>(*t.rhs);
        type = nlhs_q.type^nrhs_q.val;
    } else {
        auto nrhs = std::visit([&](auto &&c) { return resolve_type_of(c, rec_alias); }, *t.rhs);
        if (!std::get_if<resolved_quantity>(nrhs.get())) {
            throw std::runtime_error(fmt::format("Internal compiler error: expected resolved quantity type at rhs of {}",
                                                 to_string(t.loc)));
        }
        auto nrhs_q = std::get<resolved_quantity>(*nrhs);
        switch (t.op) {
            case t_binary_op::mul: type = nlhs_q.type*nrhs_q.type; break;
            case t_binary_op::div: type = nlhs_q.type/nrhs_q.type; break;
            default: break;
        }
    }
    return make_rtype<resolved_quantity>(type, t.loc);
}
r_type resolve_type_of(const integer_type& t, const std::unordered_map<std::string, r_type>&) {
    throw std::runtime_error(fmt::format("Internal compiler error: unexpected integer type at {}", to_string(t.loc)));
}
r_type resolve_type_of(const boolean_type& t, const std::unordered_map<std::string, r_type>&) {
    return make_rtype<resolved_boolean>(t.loc);
}
r_type resolve_type_of(const record_type& t, const std::unordered_map<std::string, r_type>& rec_alias) {
    std::vector<std::pair<std::string, r_type>> fields;
    for (const auto& f: t.fields) {
        fields.emplace_back(f.first, std::visit([&](auto&& c){return resolve_type_of(c, rec_alias);}, *f.second));
    }
    return make_rtype<resolved_record>(fields, t.loc);
}
r_type resolve_type_of(const record_alias_type& t, const std::unordered_map<std::string, r_type>& rec_alias) {
    if (!rec_alias.count(t.name)) {
        throw std::runtime_error(fmt::format("Undefined record {} at {}", t.name, to_string(t.loc)));
    }
    return rec_alias.at(t.name);
}

// Derive types
std::optional<r_type> derive(const resolved_quantity& q) {
    auto type = q.type;
    type.q_pow[normalized_type::q_map[quantity::time]]-=1;
    return make_rtype<resolved_quantity>(type, q.loc);
}
std::optional<r_type> derive(const resolved_boolean&) {
    return {};
}
std::optional<r_type> derive(const resolved_record& q) {
    std::vector<std::pair<std::string, r_type>> fields;
    for (auto [f_id, f_type]: q.fields) {
        auto f_prime_type = std::visit([](auto&& c){return derive(c);}, *f_type);
        if (!f_prime_type) return {};
        fields.emplace_back(f_id+"'", f_prime_type.value());
    }
    return make_rtype<resolved_record>(fields, q.loc);
}

// compare resolved_types
bool operator==(const resolved_quantity& lhs, const resolved_quantity& rhs) {
    return (lhs.type == rhs.type);
}
bool operator==(const resolved_record& lhs, const resolved_record& rhs) {
    if (lhs.fields.size() != rhs.fields.size()) return false;
    for (unsigned i = 0; i < lhs.fields.size(); ++i) {
        if (*(lhs.fields[i].second) != *(rhs.fields[i].second)) return false;
    }
    return true;
}
bool operator==(const resolved_type& lhs, const resolved_type& rhs) {
    auto bool_lhs = std::get_if<resolved_boolean>(&lhs);
    auto bool_rhs = std::get_if<resolved_boolean>(&rhs);
    if (bool_lhs && bool_rhs) return true;

    auto qty_lhs = std::get_if<resolved_quantity>(&lhs);
    auto qty_rhs = std::get_if<resolved_quantity>(&rhs);
    if (qty_lhs && qty_rhs) return *qty_lhs == *qty_rhs;

    auto rec_lhs = std::get_if<resolved_record>(&lhs);
    auto rec_rhs = std::get_if<resolved_record>(&rhs);
    if (rec_lhs && rec_rhs) return *rec_lhs == *rec_rhs;

    return false;
}
bool operator!=(const resolved_type& lhs, const resolved_type& rhs) {
    return !(lhs == rhs);
}

// to_string
std::string to_string(const normalized_type& t, int indent) {
    std::string str;
    for (auto [q, idx]: normalized_type::q_map) {
        if (auto pow = t.q_pow[idx]) {
            str += (to_string(q) + "^" + std::to_string(pow) + " ");
        }
    }
    if (str.empty()) str = "real";
    return std::string(indent*2, ' ') + str;
}
std::string to_string(const resolved_quantity& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_quantity_type\n";
    str += (to_string(q.type, indent+1) + "\n");
    return str + double_indent + to_string(q.loc) + ")";
}
std::string to_string(const resolved_boolean& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    return single_indent + "(resolved_boolean_type " + to_string(q.loc) + ")";
}
std::string to_string(const resolved_record& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent +  "(resolved_record_type\n";
    for (const auto& f: q.fields) {
        std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *(f.second));
    }
    return str + double_indent + to_string(q.loc) + ")";
}
} // namespace t_resolved_ir
} // namespace al