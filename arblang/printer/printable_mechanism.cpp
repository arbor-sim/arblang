#include <string>
#include <unordered_map>

#include <arblang/printer/printable_mechanism.hpp>
#include <arblang/resolver/resolved_expressions.hpp>

#include <fmt/core.h>

#include "../util/rexp_helpers.hpp"

namespace al {
namespace resolved_ir {

// resolved_quantity and resolved_boolean are simplified to real resolved_quantity
// resolved_record is simplified to resolved_record with real fields
// resolved_field_access is simplified to resolved_argument from the state_field_map.
r_expr simplify(const r_expr&, const state_field_map&);
r_type simplify(const r_type&);

void read_arguments(const r_expr&, std::vector<std::string>&);

printable_mechanism::printable_mechanism(const resolved_mechanism& m) {

    auto state_field_decoder = gen_state_field_map(m.states);

    // Mappings from various bind, parameter and (mangled) states
    // to their sources (the names of the pointers to the actual storage).
    // These will be used to create the read/write maps.
    std::unordered_map<std::string, std::string> variable_to_source_map;

    /**** Create variable_to_source_map; Fill field_pack ****/
    for (const auto& s: m.states) {
        auto state = std::get<resolved_state>(*s);
        auto state_name = state.name;

        auto state_rec = std::get_if<resolved_record>(state.type.get());
        if (!state_rec) {
            auto prefixed_name = prefix(state_name);
            variable_to_source_map.insert({state_name, prefixed_name});
            field_pack.state_sources.insert(prefixed_name);
        }
        else {
            for (const auto&[field_name, field_type]: state_rec->fields) {
                auto state_field_name = state_field_decoder.at(state_name).at(field_name);
                auto prefixed_state_field_name = prefix(state_field_name);
                variable_to_source_map.insert({state_field_name, prefixed_state_field_name});
                field_pack.state_sources.insert(prefixed_state_field_name);
            }
        }
    }
    for (const auto& c: m.bindings) {
        auto bind = std::get<resolved_bind>(*c);
        auto prefixed_name = prefix(bind.name);
        variable_to_source_map.insert({bind.name, prefixed_name});
        field_pack.bind_sources.insert({prefixed_name, {bind.bind, bind.ion}});
    }
    for (const auto& c: m.parameters) {
        auto param_name = std::get<resolved_parameter>(*c).name;
        auto prefixed_name = prefix(param_name);
        variable_to_source_map.insert({param_name, prefixed_name});
        field_pack.param_sources.insert(prefixed_name);
    }
    for (const auto& c: m.effects) {
        {
            auto effect = std::get<resolved_effect>(*c);
            switch (effect.effect) {
                case affectable::current_density:
                case affectable::current: {
                    throw std::runtime_error("Internal compiler error: Unexpected current/current_density "
                                             "affectable at this stage of the compilation");
                }
                case affectable::current_density_pair:
                case affectable::current_pair: {
                    auto prefixed_i_name = prefix("i");
                    auto prefixed_g_name = prefix("g");

                    auto i_effect = effect.effect == affectable::current_density_pair? affectable::current_density: affectable::current;
                    auto g_effect = effect.effect == affectable::current_density_pair? affectable::conductivity: affectable::conductance;

                    variable_to_source_map.insert({"i", prefixed_i_name});
                    field_pack.effect_sources.insert({prefixed_i_name, {i_effect, effect.ion}});

                    variable_to_source_map.insert({"g", prefixed_g_name});
                    field_pack.effect_sources.insert({prefixed_g_name, {g_effect, effect.ion}});
                    break;
                }
                default: break; // TODO when we add support for the other affectables
            }
        }
    }

    /**** Fill procedure_pack using state_field_decoder ****/
    for (const auto& c: m.parameters) {
        auto param = std::get<resolved_parameter>(*c);
        // Simplify and save the parameter declarations
        if (std::get_if<resolved_int>(param.value.get()) || std::get_if<resolved_float>(param.value.get())) {
            procedure_pack.constant_parameters.push_back(simplify(c, {}));
        }
        else {
            procedure_pack.assigned_parameters.push_back(simplify(c, {}));
        }
    }
    for (const auto& c: m.effects) {
        // Simplify and save the effects
        procedure_pack.effects.push_back(simplify(c, state_field_decoder));
    }
    for (const auto& c: m.initializations) {
        // Simplify and save the initializations
        procedure_pack.initializations.push_back(simplify(c, state_field_decoder));
    }
    for (const auto& c: m.evolutions) {
        // Simplify and save the evolutions
        procedure_pack.evolutions.push_back(simplify(c, state_field_decoder));
    }

    /**** Fill proc_write_var maps using variable_to_source_map and state_field_decoder ****/
    fill_write_maps(variable_to_source_map, state_field_decoder);

    /**** Fill proc_read_var maps using variable_to_source_map ****/
    fill_read_maps(variable_to_source_map);
}

state_field_map printable_mechanism::gen_state_field_map(const std::vector<r_expr>& state_declarations) {
    // Collect the state variables that have resolved_record type.
    // Create mangled names for the fields of the state record.
    // e.g. state s: {m: real; h:real;} -> state _s_m:real and state _s_h:real

    state_field_map decoder;
    for (const auto& s: state_declarations) {
        auto state = std::get<resolved_state>(*s);
        auto state_name = state.name;

        auto state_rec = std::get_if<resolved_record>(state.type.get());
        if (state_rec) {
            std::unordered_map<std::string, std::string> state_fields;
            for (const auto&[f_name, f_type]: state_rec->fields) {
                if (std::get_if<resolved_record>(f_type.get())) {
                    throw std::runtime_error("Internal compiler error: nested record type in state variable \n"
                                             "not yet supported.");
                }
                std::string mangled_name = "_"  + state_name + "_" + f_name;
                state_fields.insert({f_name, mangled_name});
            }
            decoder.insert({state_name, std::move(state_fields)});
        }
    }
    return std::move(decoder);
}

void printable_mechanism::fill_write_maps(
    const std::unordered_map<std::string, std::string>& var_to_source,
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& state_field_decoder)
{

    auto get_result = [](const r_expr& e) {
        if (auto let_opt = get_let(e)) {
            auto let = let_opt.value();
            return get_innermost_body(&let);
        }
        return e;
    };

    auto get_resolved_variables = [](const r_expr& e) {
        std::vector<resolved_variable> vars;
        if (auto obj = std::get_if<resolved_object>(e.get())) {
            for (const auto& f: obj->record_fields) {
                vars.push_back(std::get<resolved_variable>(*f));
            }
        }
        else if (auto var = std::get_if<resolved_variable>(e.get())) {
            vars.push_back(*var);
        }
        return vars;
    };

    for (const auto& c: procedure_pack.assigned_parameters) {
        auto param = std::get<resolved_parameter>(*c);

        if (var_to_source.count(param.name)) {
            throw std::runtime_error("Internal compiler error: can not find parameter that is being assigned.");
        }
        auto pointer_name  = var_to_source.at(param.name);

        auto vars = get_resolved_variables(get_result(param.value));
        if (vars.size() != 1) {
            throw std::runtime_error("Internal compiler error: Expected exactly one result for parameter initialization.");
        }

        // Assigned parameters are parts of the initialization
        init_write_map.parameter_map.insert({pointer_name, vars.front().name});
    }

    for (const auto& c: procedure_pack.initializations) {
        auto init = std::get<resolved_initial>(*c);

        auto state = std::get_if<resolved_argument>(init.identifier.get());
        if (!state) {
            throw std::runtime_error("Internal compiler error: expected identifier of resolved_initial to be a resolved_argument.");
        }
        std::string state_name = state->name;

        auto vars = get_resolved_variables(get_result(init.value));
        if (auto state_type = std::get_if<resolved_record>(init.type.get())) {
            if (vars.size() != state_type->fields.size()) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected {} results for state {} initialization.",
                                                     std::to_string(state_type->fields.size()), state_name));
            }
            for (const auto& v: vars) {
                // These are object fields meaning the name is the object field name
                // and the value points to another resolved_variable that is the answer
                auto field_name = v.name;
                auto field_value = get_resolved_variables(v.value);

                if (field_value.size() != 1) {
                    throw std::runtime_error(fmt::format("Internal compiler error: Expected a resolved_variable and was disappointed."));
                }

                auto field_mangled_name = state_field_decoder.at(state_name).at(field_name);
                auto pointer_name = var_to_source.at(field_mangled_name);
                init_write_map.state_map.insert({pointer_name, field_value.front().name});
            }
        }

        else {
            if (vars.size() != 1) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected 1 result for state {} initialization.",
                                                     state_name));
            }
            auto v = vars.front();

            auto pointer_name  = var_to_source.at(state_name);
            init_write_map.state_map.insert({pointer_name, vars.front().name});
        }
    }

    for (const auto& c: procedure_pack.evolutions) {
        auto evolve = std::get<resolved_evolve>(*c);

        auto state = std::get_if<resolved_argument>(evolve.identifier.get());
        if (!state) {
            throw std::runtime_error("Internal compiler error: expected identifier of resolved_evolve to be a resolved_argument.");
        }
        std::string state_name = state->name;

        auto vars = get_resolved_variables(get_result(evolve.value));
        if (auto state_type = std::get_if<resolved_record>(evolve.type.get())) {
            if (vars.size() != state_type->fields.size()) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected {} results for state {} initialization.",
                                                     std::to_string(state_type->fields.size()), state_name));
            }
            for (const auto& v: vars) {
                // These are object fields meaning the name is the object field name
                // and the value points to another resolved_variable that is the answer
                auto field_name = v.name;
                auto field_value = get_resolved_variables(v.value);

                if (field_value.size() != 1) {
                    throw std::runtime_error(fmt::format("Internal compiler error: Expected a resolved_variable and was disappointed."));
                }

                auto field_mangled_name = state_field_decoder.at(state_name).at(field_name);
                auto pointer_name = var_to_source.at(field_mangled_name);
                evolve_write_map.state_map.insert({pointer_name, field_value.front().name});
            }
        }
        else {
            if (vars.size() != 1) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected 1 result for state {} initialization.",
                                                     state_name));
            }
            auto v = vars.front();

            auto pointer_name  = var_to_source.at(state_name);
            evolve_write_map.state_map.insert({pointer_name, vars.front().name});
        }
    }

    for (const auto& c: procedure_pack.effects) {
        auto effect = std::get<resolved_effect>(*c);

        auto vars = get_resolved_variables(get_result(effect.value));
        if (auto effect_type = std::get_if<resolved_record>(effect.type.get())) {
            if (vars.size() != effect_type->fields.size()) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected {} results for state affectable {}.",
                                                     std::to_string(effect_type->fields.size()), to_string(effect.effect)));
            }
            for (const auto& v: vars) {
                // These are object fields meaning the name is the object field name
                // and the value points to another resolved_variable that is the answer
                auto field_name = v.name;
                auto field_value = get_resolved_variables(v.value);

                if (field_value.size() != 1) {
                    throw std::runtime_error(fmt::format("Internal compiler error: Expected a resolved_variable and was disappointed."));
                }

                auto pointer_name = var_to_source.at(field_name);
                effect_write_map.effect_map.insert({pointer_name, field_value.front().name});
            }
        }
    }
}

void printable_mechanism::fill_read_maps(const std::unordered_map<std::string, std::string>& var_to_source) {
    for (const auto& c: procedure_pack.assigned_parameters) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (var_to_source.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being accessed.");
            }
            auto source_a = var_to_source.at(a);

            // Assigned parameters are parts of the initialization
            if (field_pack.param_sources.count(source_a)) {
                init_read_map.parameter_map.insert({source_a, a});
            }
            else {
                throw std::runtime_error(fmt::format("Internal compiler error: parameter {} assignment requires "
                                                     "non-parameter {} read.", std::get<resolved_parameter>(*c).name, a));
            }
        }
    }

    for (const auto& c: procedure_pack.initializations) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (var_to_source.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being accessed.");
            }
            auto source_a = var_to_source.at(a);

            if (field_pack.param_sources.count(source_a)) {
                init_read_map.parameter_map.insert({source_a, a});
            }
            else if ((field_pack.bind_sources.count(source_a))) {
                init_read_map.binding_map.insert({source_a, a});
            }
            else {
                throw std::runtime_error(fmt::format("Internal compiler error: initialization requires "
                                                     "non-parameter, non-binding {} read.", a));
            }
        }
    }

    for (const auto& c: procedure_pack.evolutions) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (var_to_source.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being accessed.");
            }
            auto source_a = var_to_source.at(a);

            if (field_pack.param_sources.count(source_a)) {
                evolve_read_map.parameter_map.insert({source_a, a});
            }
            else if ((field_pack.bind_sources.count(source_a))) {
                evolve_read_map.binding_map.insert({source_a, a});
            }
            else if ((field_pack.state_sources.count(source_a))) {
                evolve_read_map.state_map.insert({source_a, a});
            }
            else {
                throw std::runtime_error(fmt::format("Internal compiler error: state evolution requires "
                                                     "non-parameter, non-binding, non-state {} read.", a));
            }
        }
    }

    for (const auto& c: procedure_pack.effects) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (var_to_source.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being accessed.");
            }
            auto source_a = var_to_source.at(a);

            if (field_pack.param_sources.count(source_a)) {
                effect_read_map.parameter_map.insert({source_a, a});
            }
            else if ((field_pack.bind_sources.count(source_a))) {
                effect_read_map.binding_map.insert({source_a, a});
            }
            else if ((field_pack.state_sources.count(source_a))) {
                effect_read_map.state_map.insert({source_a, a});
            }
            else {
                throw std::runtime_error(fmt::format("Internal compiler error: state evolution requires "
                                                     "non-parameter, non-binding, non-state {} read.", a));
            }
        }
    }
}



// simplify
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

// Get accessed resolved_arguments
void read_arguments(const resolved_record_alias& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation (after resolution).");
}
void read_arguments(const resolved_constant& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_constant at "
                             "this stage in the compilation (after optimization).");
}

void read_arguments(const resolved_function& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_function at "
                             "this stage in the compilation (after inlining).");
}

void read_arguments(const resolved_call& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_call at "
                             "this stage in the compilation (after inlining).");
}

void read_arguments(const resolved_state& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_state at "
                             "this stage in the compilation (during printing prep).");
}

void read_arguments(const resolved_bind& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_bind at "
                             "this stage in the compilation (during printing prep).");
}

void read_arguments(const resolved_export& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_export at "
                             "this stage in the compilation (during printing prep).");
}

void read_arguments(const resolved_field_access& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_export at "
                             "this stage in the compilation (during printing prep, after "
                             "simplification).");
}

void read_arguments(const resolved_parameter& e, std::vector<std::string>& vec) {
    read_arguments(e.value, vec);
}

void read_arguments(const resolved_initial& e, std::vector<std::string>& vec) {
    read_arguments(e.value, vec);
}

void read_arguments(const resolved_evolve& e, std::vector<std::string>& vec) {
    read_arguments(e.value, vec);
}

void read_arguments(const resolved_effect& e, std::vector<std::string>& vec) {
    read_arguments(e.value, vec);
}

void read_arguments(const resolved_argument& e, std::vector<std::string>& vec) {
    vec.push_back(e.name);
}

void read_arguments(const resolved_variable& e, std::vector<std::string>& vec) {}

void read_arguments(const resolved_object& e, std::vector<std::string>& vec) {
    for (const auto& a: e.field_values()) {
        read_arguments(a, vec);
    }
}

void read_arguments(const resolved_let& e, std::vector<std::string>& vec) {
    read_arguments(e.id_value(), vec);
    read_arguments(e.body, vec);
}

void read_arguments(const resolved_conditional& e, std::vector<std::string>& vec) {
    read_arguments(e.condition, vec);
    read_arguments(e.value_true, vec);
    read_arguments(e.value_false, vec);
}

void read_arguments(const resolved_float& e, std::vector<std::string>& vec) {}

void read_arguments(const resolved_int& e, std::vector<std::string>& vec) {}

void read_arguments(const resolved_unary& e, std::vector<std::string>& vec) {
    read_arguments(e.arg, vec);
}

void read_arguments(const resolved_binary& e, std::vector<std::string>& vec) {
    read_arguments(e.lhs, vec);
    read_arguments(e.rhs, vec);
}

void read_arguments(const r_expr& e, std::vector<std::string>& vec) {
    return std::visit([&](auto&& c){return read_arguments(c, vec);}, *e);
}

} // namespace resolved_ir
} // namespace al