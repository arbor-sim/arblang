#include <string>
#include <unordered_map>

#include <fmt/core.h>

#include <arblang/pre_printer/simplify.hpp>

namespace al {
namespace resolved_ir {

// Simplify
r_expr simplify(const r_expr&, const state_field_map&, std::unordered_map<std::string, r_expr>&);

r_expr simplify(const resolved_record_alias& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation (after resolution).");
}
r_expr simplify(const resolved_constant& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_constant at "
                             "this stage in the compilation (after optimization).");
}

r_expr simplify(const resolved_function& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_function at "
                             "this stage in the compilation (after inlining).");
}

r_expr simplify(const resolved_call& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_call at "
                             "this stage in the compilation (after inlining).");
}

r_expr simplify(const resolved_state& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_state at "
                             "this stage in the compilation (during printing prep).");
}

r_expr simplify(const resolved_bind& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_bind at "
                             "this stage in the compilation (during printing prep).");
}

r_expr simplify(const resolved_export& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_export at "
                             "this stage in the compilation (during printing prep).");
}

r_expr simplify(const resolved_parameter& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    auto simplified_val = simplify(e.value, map, rewrites);
    return make_rexpr<resolved_parameter>(e.name, simplified_val, type_of(simplified_val), e.loc);
}

r_expr simplify(const resolved_initial& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    auto simplified_id  = simplify(e.identifier, map, rewrites);
    auto simplified_val = simplify(e.value, map, rewrites);

    auto id_type  = type_of(simplified_id);
    auto val_type = type_of(simplified_val);

    if (*id_type != *val_type) {
        throw std::runtime_error(fmt::format("Internal compiler error, types of identifier and value of resolved_initial "
                                             "don't match after simplification: {} and {}", to_string(id_type), to_string(val_type)));
    }
    return make_rexpr<resolved_initial>(simplified_id, simplified_val, val_type, e.loc);
}

r_expr simplify(const resolved_evolve& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    auto simplified_id  = simplify(e.identifier, map, rewrites);
    auto simplified_val = simplify(e.value, map, rewrites);

    auto id_type  = type_of(simplified_id);
    auto val_type = type_of(simplified_val);

    if (*id_type != *val_type) {
        throw std::runtime_error(fmt::format("Internal compiler error, types of identifier and value of resolved_initial "
                                             "don't match after simplification: {} and {}", to_string(id_type), to_string(val_type)));
    }
    return make_rexpr<resolved_evolve>(simplified_id, simplified_val, val_type, e.loc);
}

r_expr simplify(const resolved_effect& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    auto simplified_val = simplify(e.value, map, rewrites);
    return make_rexpr<resolved_effect>(e.effect, e.ion, simplified_val, type_of(simplified_val), e.loc);
}

r_expr simplify(const resolved_argument& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    return make_rexpr<resolved_argument>(e.name, simplify(e.type), e.loc);
}

r_expr simplify(const resolved_variable& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    if (rewrites.count(e.name)) {
        return rewrites.at(e.name);
    }
    auto simple_val = simplify(e.value, map, rewrites);
    auto simple_expr = make_rexpr<resolved_variable>(e.name, simple_val, type_of(simple_val), e.loc);
    rewrites.insert({e.name, simple_expr});

    return simple_expr;
}

r_expr simplify(const resolved_object& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    std::vector<r_expr> simple_fields;
    for (const auto& a: e.field_values()) {
        simple_fields.push_back(simplify(a, map, rewrites));
    }
    auto simple_type = simplify(e.type);
    return make_rexpr<resolved_object>(e.field_names(), simple_fields, simple_type, e.loc);
}

r_expr simplify(const resolved_let& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    auto simplified_id = simplify(e.identifier, map, rewrites);
    auto simplified_body = simplify(e.body, map, rewrites);
    return make_rexpr<resolved_let>(simplified_id, simplified_body, type_of(simplified_body), e.loc);
}

r_expr simplify(const resolved_conditional& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    auto simplified_cond  = simplify(e.condition, map, rewrites);
    auto simplified_true  = simplify(e.value_true, map, rewrites);
    auto simplified_false = simplify(e.value_false, map, rewrites);

    return make_rexpr<resolved_conditional>(simplified_cond, simplified_true, simplified_false, type_of(simplified_true), e.loc);
}

r_expr simplify(const resolved_float& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    return make_rexpr<resolved_float>(e.value, simplify(e.type), e.loc);
}

r_expr simplify(const resolved_int& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    return make_rexpr<resolved_float>(e.value, simplify(e.type), e.loc);
}

r_expr simplify(const resolved_unary& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    return make_rexpr<resolved_unary>(e.op, simplify(e.arg, map, rewrites), simplify(e.type), e.loc);
}

r_expr simplify(const resolved_binary& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    auto simplified_lhs = simplify(e.lhs, map, rewrites);
    auto simplified_rhs = simplify(e.rhs, map, rewrites);
    return make_rexpr<resolved_binary>(e.op, simplified_lhs, simplified_rhs, simplify(e.type), e.loc);
}

r_expr simplify(const resolved_field_access& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    if (auto arg = std::get_if<resolved_argument>(e.object.get())) {
        // Should be referring to a state
        if (!map.count(arg->name)) {
            throw std::runtime_error(fmt::format("Internal compiler error, object of resolved_field_access "
                                                 "expected to be a state variable"));
        }
        if (!map.at(arg->name).count(e.field)) {
            throw std::runtime_error(fmt::format("Internal compiler error, field of resolved_field_access "
                                                 "expected to be a field of a state varaible"));
        }
        return make_rexpr<resolved_argument>(map.at(arg->name).at(e.field), simplify(e.type), e.loc);
    }
    throw std::runtime_error(fmt::format("Internal compiler error, object of resolved_field_access "
                                         "expected to be a resolved_argument"));}

r_expr simplify(const r_expr& e, const state_field_map& map, std::unordered_map<std::string, r_expr>& rewrites) {
    return std::visit([&](auto&& c){return simplify(c, map, rewrites);}, *e);
}

r_expr simplify(const r_expr& e, const state_field_map& map) {
    std::unordered_map<std::string, r_expr> rewrites;
    return std::visit([&](auto&& c){return simplify(c, map, rewrites);}, *e);
}

// Simpify type
r_type simplify(const resolved_quantity& q) {
    return make_rtype<resolved_quantity>(normalized_type(quantity::real), q.loc);
}

r_type simplify(const resolved_boolean& q) {
    return make_rtype<resolved_quantity>(normalized_type(quantity::real), q.loc);
}

r_type simplify(const resolved_record& q) {
    std::vector<std::pair<std::string, r_type>> fields;
    for (auto [f_id, f_type]: q.fields) {
        auto f_real_type = simplify(f_type);
        fields.emplace_back(f_id, f_real_type);
    }
    return make_rtype<resolved_record>(fields, q.loc);
}

r_type simplify(const r_type& t) {
    return std::visit([](auto&& c){return simplify(c);}, *t);
}

} // namespace resolved_ir
} // namespace al