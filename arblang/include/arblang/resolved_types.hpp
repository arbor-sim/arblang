#pragma once

#include <vector>
#include <unordered_map>

#include <arblang/type_expressions.hpp>

namespace al {
namespace t_resolved_ir {
using namespace t_raw_ir;

struct resolved_quantity;
struct resolved_boolean;
struct resolved_record;

using resolved_type = std::variant<
    resolved_quantity,
    resolved_boolean,
    resolved_record>;
using r_type = std::shared_ptr<resolved_type>;
bool operator==(const resolved_type& lhs, const resolved_type& rhs);
bool operator!=(const resolved_type& lhs, const resolved_type& rhs);

struct normalized_type {
    static std::unordered_map<quantity, int> q_map;
    std::array<int,6> q_pow = {0, 0, 0, 0, 0, 0};

    normalized_type() = default;
    normalized_type(std::array<int,6> pow): q_pow(pow) {};
    bool is_real();
    normalized_type& set(quantity, int);
};
bool operator==(const normalized_type& lhs, const normalized_type& rhs);
bool operator!=(const normalized_type& lhs, const normalized_type& rhs);

normalized_type operator*(normalized_type& lhs, normalized_type& rhs);
normalized_type operator/(normalized_type& lhs, normalized_type& rhs);
normalized_type operator^(normalized_type& lhs, int rhs);

struct resolved_quantity {
    normalized_type type;
    src_location loc;

    resolved_quantity(normalized_type t, src_location loc): type(t), loc(loc) {};
};

struct resolved_boolean {
    src_location loc;

    resolved_boolean(src_location loc): loc(loc) {};
};

struct resolved_record {
    std::vector<std::pair<std::string, r_type>> fields;
    src_location loc;

    resolved_record(std::vector<std::pair<std::string, r_type>> fields, src_location loc): fields(std::move(fields)), loc(loc) {};
};

template <typename T, typename... Args>
r_type make_rtype(Args&&... args) {
    return r_type(new resolved_type(T(std::forward<Args>(args)...)));
}

r_type resolve_type_of(const bindable& b, const src_location& loc);
r_type resolve_type_of(const affectable& a, const src_location& loc);

r_type resolve_type_of(const quantity_type&, const std::unordered_map<std::string, r_type>&);
r_type resolve_type_of(const integer_type&, const std::unordered_map<std::string, r_type>&);
r_type resolve_type_of(const quantity_binary_type&, const std::unordered_map<std::string, r_type>&);
r_type resolve_type_of(const boolean_type&, const std::unordered_map<std::string, r_type>&);
r_type resolve_type_of(const record_type&, const std::unordered_map<std::string, r_type>&);
r_type resolve_type_of(const record_alias_type&, const std::unordered_map<std::string, r_type>&);

std::optional<r_type> derive(const resolved_quantity&);
std::optional<r_type> derive(const resolved_boolean&);
std::optional<r_type> derive(const resolved_record&);

std::string to_string(const normalized_type&, int indent=0);
std::string to_string(const resolved_quantity&, int indent=0);
std::string to_string(const resolved_boolean&, int indent=0);
std::string to_string(const resolved_record&, int indent=0);

} // namespace t_resolved_ir
} // namespace al