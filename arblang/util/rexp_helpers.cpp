#include "rexp_helpers.hpp"

namespace al {
namespace resolved_ir {
r_expr get_innermost_body(resolved_let* const let) {
    resolved_let* let_last = let;
    while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
        let_last = let_next;
    }
    return let_last->body;
}

void set_innermost_body(resolved_let* const let, const r_expr& body) {
    auto body_type = type_of(body);
    resolved_let* let_last = let;
    let_last->type = body_type;
    while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
        let_last = let_next;
        let_last->type = body_type;
    }
    let_last->body = body;
}

std::optional<double> is_number(const r_expr& e) {
    if (auto v = is_resolved_float(e)) {
        return v->value;
    }
    if (auto v = is_resolved_int(e)) {
        return v->value;
    }
    return {};
}

bool is_trivial(const r_expr& e) {
    if (is_number(e)) return true;
    if (auto obj = std::get_if<resolved_object>(e.get())) {
        for (const auto& v: obj->field_values()) {
            if (!is_number(v)) return false;
        }
        return true;
    }
    return false;
}


} // namespace resolved_ir
} // namespace al
