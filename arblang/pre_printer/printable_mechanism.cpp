#include <string>
#include <unordered_map>

#include <arblang/optimizer/optimizer.hpp>
#include <arblang/pre_printer/check_mechanism.hpp>
#include <arblang/pre_printer/get_read_arguments.hpp>
#include <arblang/pre_printer/simplify.hpp>
#include <arblang/pre_printer/printable_mechanism.hpp>
#include <arblang/resolver/resolved_expressions.hpp>

#include <fmt/core.h>

#include "../util/rexp_helpers.hpp"

namespace al {
namespace resolved_ir {

// TODO Add support for
//- Param records.
//- Nested param/state records.
printable_mechanism::printable_mechanism(const resolved_mechanism& m) {
    kind = m.kind;

    // Check that the mechanism can be printed
    check(m);

    // Generate map from state.field to mangled state_field name
    auto state_field_decoder = gen_state_field_map(m.states);

    // Mappings from various bind, parameter, states and mangled states
    // to their sources (the names of the pointers to the actual storage).
    // These will be used to create the read/write maps.
    std::unordered_map<std::string, std::string> variable_to_source_map;

    /**** Generate variable_to_source_map; Fill field_pack ****/
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
        auto eff = std::get<resolved_effect>(*c);
        auto i_effect = eff.effect == affectable::current_density_pair?
                affectable::current_density: affectable::current;
        auto g_effect = eff.effect == affectable::current_density_pair?
                affectable::conductivity: affectable::conductance;

        auto prefixed_i_name = prefix("i");
        variable_to_source_map.insert({"i", prefixed_i_name});
        field_pack.effect_sources.insert({prefixed_i_name, {i_effect, eff.ion}});

        auto prefixed_g_name = prefix("g");
        variable_to_source_map.insert({"g", prefixed_g_name});
        field_pack.effect_sources.insert({prefixed_g_name, {g_effect, eff.ion}});
    }

    /**** Fill procedure_pack using state_field_decoder ****/
    for (const auto& c: m.parameters) {
        // Simplify and save the parameter declarations
        auto opt = optimizer(simplify(c, {}));
        auto simplified = opt.optimize();

        auto param_value = std::get<resolved_parameter>(*c).value;
        if (std::get_if<resolved_int>(param_value.get()) || std::get_if<resolved_float>(param_value.get())) {
            procedure_pack.constant_parameters.push_back(simplified);
        }
        else {
            procedure_pack.assigned_parameters.push_back(simplified);
        }
    }
    for (const auto& c: m.effects) {
        // Simplify and save the effects
        auto opt = optimizer(simplify(c, state_field_decoder));
        procedure_pack.effects.push_back(opt.optimize());
    }
    for (const auto& c: m.initializations) {
        // Simplify and save the initializations
        auto opt = optimizer(simplify(c, state_field_decoder));
        procedure_pack.initializations.push_back(opt.optimize());
    }
    for (const auto& c: m.evolutions) {
        // Simplify and save the evolutions
        auto opt = optimizer(simplify(c, state_field_decoder));
        procedure_pack.evolutions.push_back(opt.optimize());
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
                auto f_var = std::get_if<resolved_variable>(f.get());
                if (f_var) vars.push_back(*f_var);
            }
        }
        else if (auto var = std::get_if<resolved_variable>(e.get())) {
            vars.push_back(*var);
        }
        return vars;
    };

    auto write_var =
        [&](const std::string& vname,
            const std::vector<resolved_variable>& results,
            const r_type& type,
            std::unordered_map<std::string, std::string>& map,
            bool is_state)
   {
        if (auto vtype = std::get_if<resolved_record>(type.get())) {
            if (results.size() != vtype->fields.size()) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected {} results when writing {}.",
                                                     std::to_string(vtype->fields.size()), vname));
            }
            for (const auto& v: results) {
                // These are object fields, so v.name is the object field name
                // and the v.value points to another resolved_variable that is the answer
                auto field_name = v.name;
                auto field_value = std::get_if<resolved_variable>(v.value.get());
                if (!field_value) {
                    throw std::runtime_error(fmt::format("Internal compiler error: Expected a resolved_variable and was "
                                                         "disappointed."));
                }
                if (is_state) {
                    if (!state_field_decoder.count(vname) || !state_field_decoder.at(vname).count(field_name)) {
                        throw std::runtime_error(fmt::format("Internal compiler error: cannot find state {} with field {} "
                                                             "that is being initialized.", vname, field_name));
                    }
                    auto field_mangled_name = state_field_decoder.at(vname).at(field_name);
                    if (!var_to_source.count(field_mangled_name)) {
                        throw std::runtime_error(fmt::format("Internal compiler error: cannot find state {} with field {} "
                                                             "that is being initialized.", vname, field_name));
                    }
                    auto pointer_name = var_to_source.at(field_mangled_name);
                    map.insert({pointer_name, field_value->name});
                }
                else {
                    if (!var_to_source.count(field_name)) {
                        throw std::runtime_error("Internal compiler error: can not find parameter that is being assigned.");
                    }
                    auto pointer_name = var_to_source.at(field_name);
                    map.insert({pointer_name, field_value->name});
                }
            }
        }
        else {
            if (results.size() != 1) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected 1 result when writing {}.",
                                                     vname));
            }
            if (!var_to_source.count(vname)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being assigned.");
            }
            auto pointer_name  = var_to_source.at(vname);
            map.insert({pointer_name, results.front().name});
        }
    };

    for (const auto& c: procedure_pack.initializations) {
        auto init = std::get<resolved_initial>(*c);

        auto state_name = std::get<resolved_argument>(*init.identifier).name;
        auto state_vars = get_resolved_variables(get_result(init.value));
        auto state_type = init.type;
        write_var(state_name, state_vars, state_type, init_write_map.state_map, true);
    }

    for (const auto& c: procedure_pack.evolutions) {
        auto evolve = std::get<resolved_evolve>(*c);

        auto state_name = std::get<resolved_argument>(*evolve.identifier).name;
        auto state_vars = get_resolved_variables(get_result(evolve.value));
        auto state_type = evolve.type;
        write_var(state_name, state_vars, state_type, evolve_write_map.state_map, true);
    }

    for (const auto& c: procedure_pack.effects) {
        auto effect = std::get<resolved_effect>(*c);

        auto effect_name = to_string(effect.effect);
        auto effect_vars = get_resolved_variables(get_result(effect.value));
        auto effect_type = effect.type;
        write_var(effect_name, effect_vars, effect_type, effect_write_map.effect_map, false);
    }

    for (const auto& c: procedure_pack.assigned_parameters) {
        auto param = std::get<resolved_parameter>(*c);

        auto param_name = param.name;
        auto param_vars = get_resolved_variables(get_result(param.value));
        auto param_type = param.type;
        write_var(param_name, param_vars, param_type, init_write_map.parameter_map, false);
    }
}

void printable_mechanism::fill_read_maps(const std::unordered_map<std::string, std::string>& var_to_source) {
    for (const auto& c: procedure_pack.assigned_parameters) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (!var_to_source.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
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
            if (!var_to_source.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
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
            if (!var_to_source.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
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
            if (!var_to_source.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
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
} // namespace resolved_ir
} // namespace al