#include <string>
#include <unordered_set>

#include <arblang/canonicalize.hpp>

namespace al {
namespace resolved_ir {

static std::string unique_local_name(std::unordered_set<std::string>& reserved, std::string const& prefix = "t") {
    for (int i = 0; ; ++i) {
        std::string name = prefix + std::to_string(i) + "_";
        if (reserved.insert(name).second) {
            return name;
        }
    }
}

resolved_mechanism canonicalize(const resolved_mechanism&);

r_expr canonicalize(const resolved_parameter& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    return make_rexpr<resolved_parameter>(e.name, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_constant& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    return make_rexpr<resolved_constant>(e.name, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_state& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_state>(e);
}

r_expr canonicalize(const resolved_record_alias& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_record_alias>(e);
}

r_expr canonicalize(const resolved_function& e, std::unordered_set<std::string>& reserved) {
    auto body_canon = canonicalize(e.body, reserved);
    return make_rexpr<resolved_function>(e.name, e.args, body_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_argument& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_argument>(e);
}

r_expr canonicalize(const resolved_bind& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_bind>(e);
}

r_expr canonicalize(const resolved_initial& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    return make_rexpr<resolved_initial>(e.identifier, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_evolve& e, std::unordered_set<std::string>& reserved) {
    /// TODO: Should we be solving the ODE before we get to this step?
    auto val_canon = canonicalize(e.value, reserved);
    return make_rexpr<resolved_evolve>(e.identifier, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_effect& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    return make_rexpr<resolved_effect>(e.effect, e.ion, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_export& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_export>(e);
}

r_expr canonicalize(const resolved_call& e, std::unordered_set<std::string>& reserved) {
    std::vector<r_expr> args_canon;

    resolved_let* let_outer = nullptr;
    resolved_let* let_inner = nullptr;
    for (const auto& arg: e.call_args) {
        auto arg_canon = canonicalize(arg, reserved);
        if (auto let_first = std::get_if<resolved_let>(arg_canon.get())) {
            resolved_let* let_last = let_first;
            while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
                let_last = let_next;
            }
            args_canon.push_back(let_last->body);

            if (!let_outer) {
                let_outer = let_first;
                let_inner = let_last;
            }
            else {
                let_inner->body = arg_canon;
                let_inner = let_last;
            }
        }
        else {
            args_canon.push_back(arg_canon);
        }
    }
    auto call_canon = make_rexpr<resolved_call>(e.f_identifier, args_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_argument>(unique_local_name(reserved), e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, call_canon, temp_expr, e.type, e.loc);

    if (!let_outer) return let_wrapper;

    let_inner->body = let_wrapper;
    let_inner->type = e.type;
    return make_rexpr<resolved_let>(*let_outer);
}

r_expr canonicalize(const resolved_object& e, std::unordered_set<std::string>& reserved) {
    std::vector<r_expr> values_canon;
    resolved_let* let_outer;
    resolved_let* let_inner;

    for (const auto& arg: e.record_values) {
        auto arg_canon = canonicalize(arg, reserved);
        if (auto let_first = std::get_if<resolved_let>(arg_canon.get())) {
            resolved_let* let_last = let_first;
            while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
                let_last = let_next;
            }
            values_canon.push_back(let_last->body);

            if (!let_outer) {
                let_outer = let_first;
                let_inner = let_last;
            }
            else {
                let_inner->body = arg_canon;
                let_inner = let_last;
            }
        }
        else {
            values_canon.push_back(arg_canon);
        }
    }
    auto object_canon = make_rexpr<resolved_object>(e.r_identifier, e.record_fields, values_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_argument>(unique_local_name(reserved), e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, object_canon, temp_expr, e.type, e.loc);

    if (!let_outer) return let_wrapper;

    let_inner->body = let_wrapper;
    let_inner->type = e.type;
    return make_rexpr<resolved_let>(*let_outer);
}

r_expr canonicalize(const resolved_let& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    if (auto let_first = std::get_if<resolved_let>(val_canon.get())) {
        resolved_let* let_last = let_first;
        while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
            let_last = let_next;
        }
        auto let_outer = e;
        let_outer.value = let_last->body;
        let_last->body = make_rexpr<resolved_let>(let_outer);
        let_last->type = let_outer.type;
        return make_rexpr<resolved_let>(*let_first);
    }

    return make_rexpr<resolved_let>(e.identifier, val_canon, e.body, e.type, e.loc);
}

r_expr canonicalize(const resolved_with& e, std::unordered_set<std::string>& reserved) {
    auto val_canon = canonicalize(e.value, reserved);
    if (auto let_first = std::get_if<resolved_let>(val_canon.get())) {
        resolved_let* let_last = let_first;
        while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
            let_last = let_next;
        }
        auto with_outer = e;
        with_outer.value = let_last->body;
        let_last->body = make_rexpr<resolved_with>(with_outer);
        let_last->type = with_outer.type;
        return make_rexpr<resolved_let>(*let_first);
    }

    return make_rexpr<resolved_with>(val_canon, e.body, e.type, e.loc);
}

r_expr canonicalize(const resolved_conditional& e, std::unordered_set<std::string>& reserved) {
    auto cond_canon = canonicalize(e.condition, reserved);
    auto true_canon = canonicalize(e.value_true, reserved);
    auto false_canon = canonicalize(e.value_false, reserved);

    resolved_let* let_outer = nullptr;
    resolved_let* let_inner = nullptr;
    if (auto let_first = std::get_if<resolved_let>(cond_canon.get())) {
        resolved_let* let_last = let_first;
        while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
            let_last = let_next;
        }
        let_outer = let_first;
        let_inner = let_last;
        cond_canon = let_last->body;
    }
    if (auto let_first = std::get_if<resolved_let>(true_canon.get())) {
        resolved_let* let_last = let_first;
        while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
            let_last = let_next;
        }
        if (!let_outer) {
            let_outer = let_first;
            let_inner = let_last;
        }
        else {
            let_inner->body = true_canon;
            let_inner = let_last;
        }
        true_canon = let_last->body;
    }
    if (auto let_first = std::get_if<resolved_let>(false_canon.get())) {
        resolved_let* let_last = let_first;
        while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
            let_last = let_next;
        }
        if (!let_outer) {
            let_outer = let_first;
            let_inner = let_last;
        }
        else {
            let_inner->body = false_canon;
            let_inner = let_last;
        }
        false_canon = let_last->body;
    }

    auto if_canon = make_rexpr<resolved_conditional>(cond_canon, true_canon, false_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_argument>(unique_local_name(reserved), e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, if_canon, temp_expr, e.type, e.loc);

    if (!let_outer) return let_wrapper;

    let_inner->body = let_wrapper;
    let_inner->type = e.type;
    return make_rexpr<resolved_let>(*let_outer);
}

r_expr canonicalize(const resolved_float& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_float>(e);
}

r_expr canonicalize(const resolved_int& e, std::unordered_set<std::string>& reserved) {
    return make_rexpr<resolved_int>(e);
}

r_expr canonicalize(const resolved_unary& e, std::unordered_set<std::string>& reserved) {
    auto arg_canon = canonicalize(e.arg, reserved);

    resolved_let* let_outer = nullptr;
    resolved_let* let_inner = nullptr;
    if (auto let_first = std::get_if<resolved_let>(arg_canon.get())) {
        resolved_let* let_last = let_first;
        while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
            let_last = let_next;
        }
        let_outer = let_first;
        let_inner = let_last;
        arg_canon = let_inner->body;
    }
    auto unary_canon = make_rexpr<resolved_unary>(e.op, arg_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_argument>(unique_local_name(reserved), e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, unary_canon, temp_expr, e.type, e.loc);

    if (!let_outer) return let_wrapper;

    let_inner->body = let_wrapper;
    let_inner->type = e.type;
    return make_rexpr<resolved_let>(*let_outer);
}

r_expr canonicalize(const resolved_binary& e, std::unordered_set<std::string>& reserved) {
    auto lhs_canon = canonicalize(e.lhs, reserved);
    auto rhs_canon = canonicalize(e.rhs, reserved);

    resolved_let* let_outer = nullptr;
    resolved_let* let_inner = nullptr;
    if (auto let_first = std::get_if<resolved_let>(lhs_canon.get())) {
        resolved_let* let_last = let_first;
        while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
            let_last = let_next;
        }
        let_outer = let_first;
        let_inner = let_last;
        lhs_canon = let_inner->body;
    }
    if (auto let_first = std::get_if<resolved_let>(rhs_canon.get())) {
        resolved_let* let_last = let_first;
        while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
            let_last = let_next;
        }
        if (!let_outer) {
            let_outer = let_first;
            let_inner = let_last;
        }
        else {
            let_inner->body = rhs_canon;
            let_inner = let_last;
        }
        rhs_canon = let_last->body;
    }

    auto binary_canon = make_rexpr<resolved_binary>(e.op, lhs_canon, rhs_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_argument>(unique_local_name(reserved), e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, binary_canon, temp_expr, e.type, e.loc);

    if (!let_outer) return let_wrapper;

    let_inner->body = let_wrapper;
    let_inner->type = e.type;
    return make_rexpr<resolved_let>(*let_outer);
}

r_expr canonicalize(const r_expr& e, std::unordered_set<std::string>& reserved) {
    return std::visit([&](auto& c) {return canonicalize(c, reserved);}, *e);
}

r_expr canonicalize(const r_expr& e) {
    std::unordered_set<std::string> reserved;
    return canonicalize(e, reserved);
}

} // namespace resolved_ir
} // namespace al