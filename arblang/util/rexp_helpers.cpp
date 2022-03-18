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

std::optional<resolved_let> get_let(const r_expr& expr) {
    if (auto let = std::get_if<resolved_let>(expr.get())) {
        return *let;
    }
    return std::nullopt;
}

std::optional<double> as_number(const r_expr& e) {
    if (auto v = std::get_if<resolved_float>(e.get())) {
        return v->value;
    }
    if (auto v = std::get_if<resolved_int>(e.get())) {
        return v->value;
    }
    return {};
}

bool is_trivial(const r_expr& e) {
    if (as_number(e)) return true;
    if (auto obj = std::get_if<resolved_object>(e.get())) {
        for (const auto& v: obj->field_values()) {
            if (!as_number(v)) return false;
        }
        return true;
    }
    return false;
}


} // namespace resolved_ir
} // namespace al
