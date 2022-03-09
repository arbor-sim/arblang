#include <string>
#include <unordered_map>

#include <arblang/printer/printable_mechanism.hpp>
#include <arblang/printer/simplify.hpp>
#include <arblang/resolver/resolved_expressions.hpp>

#include <fmt/core.h>

#include "../util/rexp_helpers.hpp"

namespace al {
namespace resolved_ir {

using field_map = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

// resolved_quantity and resolved_boolean are simplified to real resolved_quantity
// resolved_record is simplified to resolved_record with real fields
// resolved_field_access is simplified to resolved_argument from the field_map.
// resolved_parameter::value becomes a resolved_let if not already a resolved_int or resolved_float
r_expr simplify(const r_expr& m, const field_map& map);

// resolved_quantity and resolved_boolean are simplified to real resolved_quantity
// resolved_record is simplified to resolved_record with real fields
// resolved_field_access is simplified to resolved_argument from the field_map.
// resolved_parameter::value is a resolved_let if not a resolved_int or resolved_float
void accessed_resolved_arguments(const r_expr& m, std::vector<std::string>& map);

printable_mechanism simplify(const resolved_mechanism& m) {
    printable_mechanism mech;

    // Collect the state variables that have resolved_record type.
    // Create mangled names for the fields of the state record.
    // e.g. state s: {m: real; h:real;} -> state _s_m:real and state _s_h:real
    // These mangled names will be used to create resolved_argument to replace
    // resolved_field_access in `simplify`.
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> state_field_map;

    // Also collect mappings from various bind, parameter and (mangled) states
    // to their sources - the sources will ultimately be the names of the pointers
    // to the actual storage.
    // These will be used to create the read/write maps.
    std::unordered_map<std::string, std::string> variable_to_source_map;

    for (const auto& s: m.states) {
        auto state = std::get<resolved_state>(*s);
        auto state_name = state.name;

        auto state_rec = std::get_if<resolved_record>(state.type.get());
        if (!state_rec) {
            auto prefixed_name = mech.pp_prefix + state_name;

            // Save the mapping from state_name to prefixed_name.
            // This will be used to create the read/write maps.
            variable_to_source_map.insert({state_name, prefixed_name});

            // Save the prefixed state name.
            // This will be used for the pointer name during printing.
            mech.field_pack.state_sources.push_back(prefixed_name);
        }
        else {
            std::unordered_map<std::string, std::string> state_fields;
            for (const auto&[f_name, f_type]: state_rec->fields) {
                std::string mangled_name = "_" + state_name + "_" + f_name;
                state_fields.insert({f_name, mangled_name});

                auto prefixed_name = mech.pp_prefix + state_name + "_" + f_name;

                // Save the mapping from mangled field name to prefixed_name.
                // This will be used to create the read/write maps.
                variable_to_source_map.insert({mangled_name, prefixed_name});

                // Save the prefixed state field name.
                // This will be used for the pointer name during printing.
                mech.field_pack.state_sources.push_back(prefixed_name);
            }
            state_field_map.insert({state_name, std::move(state_fields)});
        }
    }

    for (const auto& c: m.parameters) {
        auto param = std::get<resolved_parameter>(*c);

        // Simplify and save the parameter declarations
        if (std::get_if<resolved_int>(param.value.get()) || std::get_if<resolved_float>(param.value.get())) {
            mech.constant_parameters.push_back(simplify(c, {}));
        }
        else {
            mech.assigned_parameters.push_back(simplify(c, {}));
        }

        auto prefixed_name = mech.pp_prefix+param.name;

        // Save the mapping from parameter name to prefixed_name.
        // This will be used to create the read/write maps.
        variable_to_source_map.insert({param.name, prefixed_name});

        // Save the prefixed parameter name.
        // This will be used for the pointer name during printing.
        mech.field_pack.state_sources.push_back(prefixed_name);
    }

    for (const auto& c: m.bindings) {
        auto bind = std::get<resolved_bind>(*c);

        auto prefixed_name = mech.pp_prefix+bind.name;

        // Save the mapping from binding name to prefixed_name.
        // This will be used to create the read/write maps.
        variable_to_source_map.insert({bind.name, prefixed_name});

        // Save the prefixed binding name.
        // This will be used for the pointer name during printing.
        mech.field_pack.bind_sources.emplace_back(bind.bind, bind.ion, prefixed_name);
    }

    for (const auto& c: m.initializations) {
        // Simplify and save the initializations
        mech.initializations.push_back(simplify(c, state_field_map));
    }
    for (const auto& c: m.evolutions) {
        // Simplify and save the evolutions
        mech.evolutions.push_back(simplify(c, state_field_map));
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
                    auto prefixed_i_name = mech.pp_prefix+"i";
                    auto prefixed_g_name = mech.pp_prefix+"g";

                    // Save the mapping from `i` and `g` to their prefixed_name.
                    // These will be used to create the read/write maps.
                    variable_to_source_map.insert({"i", prefixed_i_name});
                    variable_to_source_map.insert({"g", prefixed_g_name});

                    auto i_effect = effect.effect == affectable::current_density_pair? affectable::current_density: affectable::current;
                    auto g_effect = effect.effect == affectable::current_density_pair? affectable::conductivity: affectable::conductance;

                    // Save the prefixed `i` and `g` names.
                    // These will be used for the pointer names during printing.
                    mech.field_pack.effect_sources.emplace_back(i_effect, effect.ion, prefixed_i_name);
                    mech.field_pack.effect_sources.emplace_back(g_effect, effect.ion, prefixed_g_name);
                    break;
                }
                default: break; // TODO when we add support for the other affectables
            }
        }

        // Simplify and save the effects
        mech.effects.push_back(simplify(c, state_field_map));
    }

    // Finally, create the write/read maps for each of the newly simplified
    // parameters, initializations, evolutions, and effects

    // Start with the write maps
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

    for (const auto& c: mech.assigned_parameters) {
        auto param = std::get<resolved_parameter>(*c);

        if (variable_to_source_map.count(param.name)) {
            throw std::runtime_error("Internal compiler error: can not find parameter that is being assigned.");
        }
        auto pointer_name  = variable_to_source_map.at(param.name);

        auto vars = get_resolved_variables(get_result(param.value));
        if (vars.size() != 1) {
            throw std::runtime_error("Internal compiler error: Expected exactly one result for parameter initialization.");
        }

        // Assigned parameters are parts of the initialization
        mech.init_write_map.parameter_map.insert({pointer_name, vars.front().name});
    }

    for (const auto& c: mech.initializations) {
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

                auto field_mangled_name = state_field_map.at(state_name).at(field_name);
                auto pointer_name = variable_to_source_map.at(field_mangled_name);
                mech.init_write_map.state_map.insert({pointer_name, field_value.front().name});
            }
        }
        else {
            if (vars.size() != 1) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected 1 result for state {} initialization.",
                                                     state_name));
            }
            auto v = vars.front();

            auto pointer_name  = variable_to_source_map.at(state_name);
            mech.init_write_map.state_map.insert({pointer_name, vars.front().name});
        }
    }
    for (const auto& c: mech.evolutions) {
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

                auto field_mangled_name = state_field_map.at(state_name).at(field_name);
                auto pointer_name = variable_to_source_map.at(field_mangled_name);
                mech.evolve_write_map.state_map.insert({pointer_name, field_value.front().name});
            }
        }
        else {
            if (vars.size() != 1) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected 1 result for state {} initialization.",
                                                     state_name));
            }
            auto v = vars.front();

            auto pointer_name  = variable_to_source_map.at(state_name);
            mech.evolve_write_map.state_map.insert({pointer_name, vars.front().name});
        }
    }
    for (const auto& c: mech.effects) {
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

                auto pointer_name = variable_to_source_map.at(field_name);
                mech.effect_write_map.effect_map.insert({pointer_name, field_value.front().name});
            }
        }
        // TODO when we add support for other affectables
    }

    // Then the read maps

    return mech;
}

} // namespace resolved_ir
} // namespace al