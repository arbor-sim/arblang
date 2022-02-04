#include <arblang/canonicalize.hpp>

namespace al {
namespace resolved_ir {

resolved_mechanism canonicalize(const resolved_mechanism&);

r_expr canonicalize (const resolved_parameter& e) {
    auto val_canon = canonicalize(e.value);
    return make_rexpr<resolved_parameter>(e.name, val_canon, e.type, e.loc);
}

r_expr canonicalize (const resolved_constant& e) {
    auto val_canon = canonicalize(e.value);
    return make_rexpr<resolved_constant>(e.name, val_canon, e.type, e.loc);
}

r_expr canonicalize (const resolved_state& e) {
    return make_rexpr<resolved_state>(e);
}

r_expr canonicalize (const resolved_record_alias& e) {
    return make_rexpr<resolved_record_alias>(e);
}

r_expr canonicalize (const resolved_function& e) {
    auto body_canon = canonicalize(e.body);
    return make_rexpr<resolved_function>(e.name, e.args, body_canon, e.type, e.loc);
}

r_expr canonicalize (const resolved_argument& e) {
    return make_rexpr<resolved_argument>(e);
}

r_expr canonicalize (const resolved_bind& e) {
    return make_rexpr<resolved_bind>(e);
}

r_expr canonicalize (const resolved_initial& e) {
    auto val_canon = canonicalize(e.value);
    return make_rexpr<resolved_initial>(e.identifier, val_canon, e.type, e.loc);
}

r_expr canonicalize (const resolved_evolve& e) {
    /// TODO: Should we be solving the ODE before we get to this step?
    auto val_canon = canonicalize(e.value);
    return make_rexpr<resolved_evolve>(e.identifier, val_canon, e.type, e.loc);
}

r_expr canonicalize (const resolved_effect& e) {
    auto val_canon = canonicalize(e.value);
    return make_rexpr<resolved_effect>(e.effect, e.ion, val_canon, e.type, e.loc);
}

r_expr canonicalize (const resolved_export& e) {
    return make_rexpr<resolved_export>(e);
}

r_expr canonicalize (const resolved_call& e) {
    std::vector<r_expr> args_canon;
    resolved_let* let_canon;
    bool is_let = false;

/*    if (auto let_first = std::get_if<resolved_let>(val_canon.get())) {
        resolved_let* let_last = let_first;
        while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
            let_last = let_next;
        }
        auto let_outer = e;
        let_outer.value = let_last->body;
        let_last->body = make_rexpr<resolved_let>(let_outer);
        let_last->type = let_outer.type;
        return make_rexpr<resolved_let>(*let_first);
    }*/

    for (const auto& arg: e.call_args) {
        auto c_arg = canonicalize(arg);
        if (auto let_first = std::get_if<resolved_let>(c_arg.get())) {
            if (!is_let) {
                let_canon = let_first;
            }
            else {
                let_canon.body = c_arg;
            }
            is_let = true;
        }
        else {
            args_canon.push_back(c_arg);
        }
    }
    auto call_canon = make_rexpr<resolved_call>(e.f_identifier, args_canon, e.type, e.loc);

    if (!is_let) return call_canon;

    let_canon.body = call_canon;
    let_canon.type = e.type;
    return make_rexpr<resolved_let>(let_canon);
}

r_expr canonicalize (const resolved_object& e) {
    std::vector<r_expr> fields_canon;
    resolved_let let_canon;
    bool is_let = false;

    for (const auto& arg: e.record_values) {
        auto c_arg = canonicalize(arg);
        if (auto let_c = std::get_if<resolved_let>(c_arg.get())) {
            fields_canon.push_back(let_c->body);
            if (!is_let) {
                let_canon = *let_c;
            }
            else {
                let_canon.body = c_arg;
            }
            is_let = true;
        }
        else {
            fields_canon.push_back(c_arg);
        }
    }
    auto object_canon = make_rexpr<resolved_object>(e.r_identifier, e.record_fields, fields_canon, e.type, e.loc);

    if (!is_let) return object_canon;

    let_canon.body = object_canon;
    let_canon.type = e.type;
    return make_rexpr<resolved_let>(let_canon);
}

r_expr canonicalize (const resolved_let& e) {
    auto val_canon = canonicalize(e.value);
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

r_expr canonicalize (const resolved_with& e) {}

r_expr canonicalize (const resolved_conditional& e) {}

r_expr canonicalize (const resolved_float& e) {}

r_expr canonicalize (const resolved_int& e) {}

r_expr canonicalize (const resolved_unary& e) {}

r_expr canonicalize (const resolved_binary& e) {}

r_expr canonicalize(const r_expr& e) {
    return std::visit([](auto& c) {return canonicalize(c);}, *e);
}

} // namespace resolved_ir
} // namespace al