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
struct resolved_alias;

using resolved_type = std::variant<
    resolved_quantity,
    resolved_boolean,
    resolved_record,
    resolved_alias>;
using r_type = std::shared_ptr<resolved_type>;

struct normalized_type {
    static std::unordered_map<quantity, int> q_map;
    std::array<int,6> q_pow = {0, 0, 0, 0, 0, 0};

    normalized_type() = default;
    normalized_type(std::array<int,6> pow): q_pow(pow) {};
    bool is_real();
    normalized_type& set(quantity, int);
};
bool operator==(normalized_type& lhs, normalized_type& rhs);
bool operator!=(normalized_type& lhs, normalized_type& rhs);

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

struct resolved_alias {
    std::string name;
    src_location loc;

    resolved_alias(std::string name, src_location loc): name(std::move(name)), loc(loc) {};
};

template <typename T, typename... Args>
r_type make_rtype(Args&&... args) {
    return r_type(new resolved_type(T(std::forward<Args>(args)...)));
}

r_type resolve_type_of(const bindable& b, const src_location& loc);
r_type resolve_type_of(const affectable& a, const src_location& loc);

r_type resolve_type_of(const quantity_type&);
r_type resolve_type_of(const integer_type&);
r_type resolve_type_of(const quantity_binary_type&);
r_type resolve_type_of(const boolean_type&);
r_type resolve_type_of(const record_type&);
r_type resolve_type_of(const record_alias_type&);

} // namespace t_resolved_ir
} // namespace al