#include <string>
#include <unordered_map>

#include <arblang/resolver/canonicalize.hpp>
#include <arblang/optimizer/optimizer.hpp>
#include <arblang/solver/solve_ode.hpp>
#include <arblang/solver/symbolic_diff.hpp>
#include <arblang/util/pretty_printer.hpp>

#include "../util/rexp_helpers.hpp"

namespace al {
namespace resolved_ir {

resolved_evolve solve_ode(const resolved_evolve& e) {
    // Get b

    // Get the type and name of the state and create an equivalent zero-valued
    // resolved_variable or resolved_argument.
    // state is expected to be a resolved_argument

    auto state_type = type_of(e.identifier);
    auto state_loc = location_of(e.identifier);
    std::string state_name;
    if (auto s = std::get_if<resolved_argument>(e.identifier.get())) {
        state_name = s->name;
    }
    else {
        throw std::runtime_error("Internal compiler error, expected a resolved_argument as the "
                                 "identifier of the resolved_evolve at " + to_string(state_loc));
    }

    r_expr zero_state;
    bool state_rec = false;
    if (auto t = std::get_if<resolved_record>(state_type.get())) {
        std::vector<r_expr> fields;
        for (const auto& [f_id, f_type]: t->fields) {
            auto zero = make_rexpr<resolved_int>(0, f_type, state_loc);
            fields.push_back(make_rexpr<resolved_variable>(f_id, zero, f_type, state_loc));
        }
        zero_state = make_rexpr<resolved_object>(fields, state_type, state_loc);
        state_rec = true;
    }
    else if (auto t = std::get_if<resolved_quantity>(state_type.get())) {
        zero_state = make_rexpr<resolved_int>(0, state_type, state_loc);
    }

    std::unordered_map<std::string, r_expr> copy_map = {{state_name, zero_state}};
    std::unordered_map<std::string, r_expr> rewrite_map = {};

    auto e_copy = copy_propagate(make_rexpr<resolved_evolve>(e), copy_map, rewrite_map);

    auto opt = optimizer(e_copy.first);
    auto e_constant = opt.optimize();

    // Get a
    r_expr evolve_result = e.value;
    if (auto let = std::get_if<resolved_let>(e.value.get())) {
        evolve_result = get_innermost_body(let);
    }

    r_expr e_deriv;
    if (state_rec) {
        if (auto obj = std::get_if<resolved_object>(evolve_result.get())) {
            std::vector<r_expr> field_deriv;
            for (auto field: obj->record_fields) {
                if (auto fld = std::get_if<resolved_variable>(field.get())) {
                    auto f_name = fld->name;
                    if (f_name.back() != '\'') {
                        throw std::runtime_error("Internal compiler error, expected a \' as the identifier of the state_field at " + to_string(obj->loc));
                    }
                    f_name.pop_back();

                    field_deriv.push_back(make_rexpr<resolved_variable>(fld->name, sym_diff(fld->value, state_name, f_name), fld->type, fld->loc));
                }
                else {
                    throw std::runtime_error("Internal compiler error, expected a resolved_varialbe as the "
                                             "field of the resolved_object at " + to_string(obj->loc));
                }
            }
            e_deriv = make_rexpr<resolved_object>(field_deriv, obj->type, obj->loc);
        }
        else {
            throw std::runtime_error("Internal compiler error, expected a resolved_object as the "
                                     "result of the resolved_evolve at " + to_string(state_loc));
        }
    }
    else {
        e_deriv = sym_diff(evolve_result, state_name);
    }

    e_deriv = canonicalize(e_deriv, "d");

    auto opt_a = optimizer(e_deriv);
    e_deriv = opt_a.optimize();

    std::cout << pretty_print(e_deriv) << std::endl;

    return resolved_evolve(e);
}

} // namespace resolved_ir
} // namespace al