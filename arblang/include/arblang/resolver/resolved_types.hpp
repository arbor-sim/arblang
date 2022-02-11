#pragma once

#include <vector>
#include <unordered_map>

#include <arblang/parser/parsed_types.hpp>

namespace al {
namespace resolved_type_ir {
using namespace parsed_type_ir;

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
    std::array<int,6> quantity_exponents = {0, 0, 0, 0, 0, 0};

    normalized_type() = default;
    normalized_type(quantity q);
    normalized_type(std::array<int,6> pow): quantity_exponents(pow) {};

    bool is_real();
    normalized_type& set(quantity, int);
    int get(quantity);
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

r_type resolve_type(const bindable& b, const src_location& loc);
r_type resolve_type(const affectable& a, const src_location& loc);
r_type resolve_type(const p_type&, const std::unordered_map<std::string, r_type>&);

std::optional<r_type> derive(const r_type&);

std::string to_string(const normalized_type&, int indent=0);
std::string to_string(const r_type&, int indent=0);

} // namespace resolved_type_ir
} // namespace al