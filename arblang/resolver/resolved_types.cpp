#include <stdexcept>

#include <fmt/core.h>

#include <arblang/parser/parsed_types.hpp>
#include <arblang/resolver/resolved_types.hpp>

namespace al {
namespace resolved_type_ir {
using namespace parsed_type_ir;

// normalized_type
normalized_type::normalized_type(quantity q) {
    switch (q) {
        //                                                   m kg  s  A mol K
        case quantity::real:          quantity_exponents = { 0, 0, 0, 0, 0, 0}; break;
        case quantity::length:        quantity_exponents = { 1, 0, 0, 0, 0, 0}; break;
        case quantity::mass:          quantity_exponents = { 0, 1, 0, 0, 0, 0}; break;
        case quantity::time:          quantity_exponents = { 0, 0, 1, 0, 0, 0}; break;
        case quantity::current:       quantity_exponents = { 0, 0, 0, 1, 0, 0}; break;
        case quantity::amount:        quantity_exponents = { 0, 0, 0, 0, 1, 0}; break;
        case quantity::temperature:   quantity_exponents = { 0, 0, 0, 0, 0, 1}; break;
        case quantity::charge:        quantity_exponents = { 0, 0, 1, 1, 0, 0}; break;
        case quantity::frequency:     quantity_exponents = { 0, 0,-1, 0, 0, 0}; break;
        case quantity::voltage:       quantity_exponents = { 2, 1,-3,-1, 0, 0}; break;
        case quantity::resistance:    quantity_exponents = { 2, 1,-3,-2, 0, 0}; break;
        case quantity::conductance:   quantity_exponents = {-2,-1, 3, 2, 0, 0}; break;
        case quantity::capacitance:   quantity_exponents = {-2,-1, 4, 2, 0, 0}; break;
        case quantity::inductance:    quantity_exponents = { 2, 1,-2,-2, 0, 0}; break;
        case quantity::force:         quantity_exponents = { 1, 1,-2, 0, 0, 0}; break;
        case quantity::pressure:      quantity_exponents = {-1, 1,-2, 0, 0, 0}; break;
        case quantity::energy:        quantity_exponents = { 2, 1,-2, 0, 0, 0}; break;
        case quantity::power:         quantity_exponents = { 2, 1,-3, 0, 0, 0}; break;
        case quantity::area:          quantity_exponents = { 2, 0, 0, 0, 0, 0}; break;
        case quantity::volume:        quantity_exponents = { 3, 0, 0, 0, 0, 0}; break;
        case quantity::concentration: quantity_exponents = {-3, 0, 0, 0, 1, 0}; break;
    }
};
bool normalized_type::is_real() {
    return std::all_of(quantity_exponents.begin(), quantity_exponents.end(), [](int i){return i==0;});
}
normalized_type& normalized_type::set(quantity q, int val) {
    switch (q) {
        case quantity::length:         quantity_exponents[0] = val; break;
        case quantity::mass:           quantity_exponents[1] = val; break;
        case quantity::time:           quantity_exponents[2] = val; break;
        case quantity::current:        quantity_exponents[3] = val; break;
        case quantity::amount:         quantity_exponents[4] = val; break;
        case quantity::temperature:    quantity_exponents[5] = val; break;
        default: throw std::runtime_error("Internal compiler error: expected base SI quantity");
    }
    return *this;
}
int normalized_type::get(quantity q) {
    switch (q) {
        case quantity::length:         return quantity_exponents[0];
        case quantity::mass:           return quantity_exponents[1];
        case quantity::time:           return quantity_exponents[2];
        case quantity::current:        return quantity_exponents[3];
        case quantity::amount:         return quantity_exponents[4];
        case quantity::temperature:    return quantity_exponents[5];
        default: throw std::runtime_error("Internal compiler error: expected base SI quantity");
    }
}

bool operator==(const normalized_type& lhs, const normalized_type& rhs) {
    return lhs.quantity_exponents == rhs.quantity_exponents;
}
bool operator!=(const normalized_type& lhs, const normalized_type& rhs) {
    return !(lhs.quantity_exponents == rhs.quantity_exponents);
}
normalized_type operator*(const normalized_type& lhs, const normalized_type& rhs) {
    normalized_type t = lhs;
    for (unsigned i = 0; i < 6; ++i) {
        t.quantity_exponents[i] += rhs.quantity_exponents[i];
    }
    return t;
}
normalized_type operator/(const normalized_type& lhs, const normalized_type& rhs) {
    normalized_type t = lhs;
    for (unsigned i = 0; i < 6; ++i) {
        t.quantity_exponents[i] -= rhs.quantity_exponents[i];
    }
    return t;
}
normalized_type operator^(const normalized_type& lhs, int rhs) {
    normalized_type t = lhs;
    for (unsigned i = 0; i < 6; ++i) {
        t.quantity_exponents[i] *= rhs;
    }
    return t;
}

// Resolve types
r_type resolve_type(const bindable& b, const src_location& loc) {
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

r_type resolve_type(const affectable& a, const src_location& loc) {
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

r_type resolve_type(const parsed_quantity_type& t, const std::unordered_map<std::string, r_type>&) {
    return make_rtype<resolved_quantity>(normalized_type(t.type), t.loc);
}
r_type resolve_type(const parsed_binary_quantity_type& t, const std::unordered_map<std::string, r_type>& rec_alias) {
    auto qlhs = is_resolved_quantity_type(resolve_type(t.lhs, rec_alias));
    if (!qlhs) {
        throw std::runtime_error(fmt::format("Internal compiler error: expected resolved quantity type at lhs of {}",
                                             to_string(t.loc)));
    }
    auto lhs_type = qlhs->type;

    normalized_type type;
    if (t.op == t_binary_op::pow) {
        auto qrhs = is_parsed_integer_type(t.rhs);
        if (!qrhs) {
            throw std::runtime_error(fmt::format("Internal compiler error: expected integer type at rhs of {}",
                                                 to_string(t.loc)));
        }
        auto rhs_val = qrhs->val;
        type = lhs_type^rhs_val;
    } else {
        auto qrhs = is_resolved_quantity_type(resolve_type(t.rhs, rec_alias));
        if (!qrhs) {
            throw std::runtime_error(fmt::format("Internal compiler error: expected resolved quantity type at rhs of {}",
                                                 to_string(t.loc)));
        }
        auto rhs_type = qrhs->type;
        switch (t.op) {
            case t_binary_op::mul: type = lhs_type*rhs_type; break;
            case t_binary_op::div: type = lhs_type/rhs_type; break;
            default: break;
        }
    }
    return make_rtype<resolved_quantity>(type, t.loc);
}
r_type resolve_type(const parsed_integer_type& t, const std::unordered_map<std::string, r_type>&) {
    throw std::runtime_error(fmt::format("Internal compiler error: unexpected integer type at {}", to_string(t.loc)));
}
r_type resolve_type(const parsed_bool_type& t, const std::unordered_map<std::string, r_type>&) {
    return make_rtype<resolved_boolean>(t.loc);
}
r_type resolve_type(const parsed_record_type& t, const std::unordered_map<std::string, r_type>& rec_alias) {
    std::vector<std::pair<std::string, r_type>> fields;
    for (const auto& f: t.fields) {
        fields.emplace_back(f.first, resolve_type(f.second, rec_alias));
    }
    return make_rtype<resolved_record>(fields, t.loc);
}
r_type resolve_type(const parsed_record_alias_type& t, const std::unordered_map<std::string, r_type>& rec_alias) {
    if (!rec_alias.count(t.name)) {
        throw std::runtime_error(fmt::format("Undefined record {} at {}", t.name, to_string(t.loc)));
    }
    return rec_alias.at(t.name);
}

r_type resolve_type(const p_type& t, const std::unordered_map<std::string, r_type>& map) {
    return std::visit([&](auto&& c){return resolve_type(c, map);}, *t);
}

// Derive types
std::optional<r_type> derive(const resolved_quantity& q) {
    auto type = q.type;
    type.set(quantity::time, type.get(quantity::time)-1);
    return make_rtype<resolved_quantity>(type, q.loc);
}
std::optional<r_type> derive(const resolved_boolean&) {
    return {};
}
std::optional<r_type> derive(const resolved_record& q) {
    std::vector<std::pair<std::string, r_type>> fields;
    for (auto [f_id, f_type]: q.fields) {
        auto f_prime_type = derive(f_type);
        if (!f_prime_type) return {};
        fields.emplace_back(f_id+"'", f_prime_type.value());
    }
    return make_rtype<resolved_record>(fields, q.loc);
}
std::optional<r_type> derive(const r_type& t) {
    return std::visit([](auto&& c){return derive(c);}, *t);
}

// compare resolved_types
bool operator==(const resolved_quantity& lhs, const resolved_quantity& rhs) {
    return (lhs.type == rhs.type);
}
bool operator==(const resolved_record& lhs, const resolved_record& rhs) {
    auto lhs_fields = lhs.fields;
    auto rhs_fields = rhs.fields;
    std::sort(lhs_fields.begin(), lhs_fields.end(), [](const auto& a, const auto&b) {return a.first < b.first;});
    std::sort(rhs_fields.begin(), rhs_fields.end(), [](const auto& a, const auto&b) {return a.first < b.first;});

    if (lhs_fields.size() != rhs_fields.size()) return false;
    for (unsigned i = 0; i < lhs_fields.size(); ++i) {
        if (*(lhs_fields[i].second) != *(rhs_fields[i].second)) return false;
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
    if (auto val = t.quantity_exponents[0]) str += "m^" +      std::to_string(val) + "*";
    if (auto val = t.quantity_exponents[1]) str += "Kg^" +        std::to_string(val) + "*";
    if (auto val = t.quantity_exponents[2]) str += "s^" +        std::to_string(val) + "*";
    if (auto val = t.quantity_exponents[3]) str += "A^" +     std::to_string(val) + "*";
    if (auto val = t.quantity_exponents[4]) str += "mol^" +      std::to_string(val) + "*";
    if (auto val = t.quantity_exponents[5]) str += "K^" + std::to_string(val) + "*";
    if (str.empty()) {
        str = "real";
    }
    else {
        str.pop_back();
    }
    return std::string(indent*2, ' ') + str;
}
std::string to_string(const resolved_quantity& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_parsed_quantity_type\n";
    return str += (to_string(q.type, indent+1) + ")");
}
std::string to_string(const resolved_boolean& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    return single_indent + "(resolved_parsed_bool_type)";
}
std::string to_string(const resolved_record& q, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent +  "(resolved_parsed_record_type\n";
    for (const auto& f: q.fields) {
        str += double_indent + f.first + "\n";
        std::visit([&](auto&& c) {str += (to_string(c, indent+1) + "\n");}, *(f.second));
    }
    return str + double_indent + ")";
}
std::string to_string(const r_type& q, int indent) {
    return std::visit([&](auto&& c){return to_string(c, indent);}, *q);
}
src_location location_of(const r_type& e) {
    return std::visit([](auto&& c){return c.loc;}, *e);
}

std::optional<resolved_quantity> is_resolved_quantity_type(const r_type& r) {
    if (!std::holds_alternative<resolved_quantity>(*r)) return {};
    return std::get<resolved_quantity>(*r);
}
std::optional<resolved_boolean> is_resolved_bool_type(const r_type& r) {
    if (!std::holds_alternative<resolved_boolean>(*r)) return {};
    return std::get<resolved_boolean>(*r);
}
std::optional<resolved_record> is_resolved_record_type(const r_type& r) {
    if (!std::holds_alternative<resolved_record>(*r)) return {};
    return std::get<resolved_record>(*r);
}

} // namespace resolved_type_ir
} // namespace al