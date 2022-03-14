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
    name = m.name;

    // Check that the mechanism can be printed
    check(m);

    // Generate map from state.field to mangled state_field name
    auto state_field_decoder = gen_state_field_map(m.states);

    // Maps used to track the type of a read/write variable
    std::unordered_set<std::string> param_set, state_set, bind_set, effect_set;

    /**** Fill pointer_map and field_pack ****/
    for (const auto& s: m.states) {
        auto state = std::get<resolved_state>(*s);
        auto state_name = state.name;

        auto state_rec = std::get_if<resolved_record>(state.type.get());
        if (!state_rec) {
            pointer_map.insert({state_name, prefix(state_name)});
            field_pack.state_sources.push_back(state_name);
            state_set.insert(state_name);
        }
        else {
            for (const auto&[field_name, field_type]: state_rec->fields) {
                auto state_field_name = state_field_decoder.at(state_name).at(field_name);
                pointer_map.insert({state_field_name, prefix(state_field_name)});
                field_pack.state_sources.push_back(state_field_name);
                state_set.insert(state_field_name);
            }
        }
    }
    std::unordered_map<std::string, unsigned> ion_idx;
    for (const auto& c: m.bindings) {
        auto bind = std::get<resolved_bind>(*c);
        auto bind_name = bind.name;
        if (bind.ion) bind_name += ("_" + bind.ion.value());

        pointer_map.insert({bind_name, prefix(bind_name)});
        field_pack.bind_sources.push_back({bind_name, bind.bind, bind.ion});
        bind_set.insert(bind_name);

        if (bind.ion) {
            auto ion = bind.ion.value();
            bool reads_charge = bind.bind == bindable::charge;
            bool writes_iconc = bind.bind == bindable::internal_concentration;
            bool writes_econc = bind.bind == bindable::external_concentration;

            auto it = ion_idx.find(ion);
            if (it != ion_idx.end()) {
                if (reads_charge) ionic_fields[it->second].read_valence = true;
                if (writes_iconc) ionic_fields[it->second].write_ext_concentration = true;
                if (writes_econc) ionic_fields[it->second].write_int_concentration = true;
            } else {
                ion_idx[ion] = ionic_fields.size();
                ionic_fields.push_back({ion, reads_charge, writes_iconc, writes_econc});
            };
        }
    }
    for (const auto& c: m.parameters) {
        auto param = std::get<resolved_parameter>(*c);
        pointer_map.insert({param.name, prefix(param.name)});

        double val = NAN;
        if (auto num = std::get_if<resolved_int>(param.value.get())) val = num->value;
        if (auto num = std::get_if<resolved_float>(param.value.get())) val = num->value;

        auto type = std::get<resolved_quantity>(*param.type).type;
        field_pack.param_sources.emplace_back(param.name, val, to_string(type));
        param_set.insert(param.name);
    }
    for (const auto& c: m.effects) {
        auto eff = std::get<resolved_effect>(*c);
        auto i_effect = eff.effect == affectable::current_density_pair?
                affectable::current_density: affectable::current;
        auto g_effect = eff.effect == affectable::current_density_pair?
                affectable::conductivity: affectable::conductance;

        std::string i_name = "i";
        std::string g_name = "g";

        // effects on i(ion or no ion) always affect overall i
        if (!effect_set.count(i_name)) {
            pointer_map.insert({i_name, prefix(i_name)});
            field_pack.effect_sources.emplace_back(i_name, i_effect, eff.ion);
            effect_set.insert(i_name);
        }
        // effects on g(ion or no ion) only affect overall g
        if (!effect_set.count(g_name)) {
            pointer_map.insert({g_name, prefix(g_name)});
            field_pack.effect_sources.emplace_back(g_name, g_effect, eff.ion);
            effect_set.insert(g_name);
        }

        if (eff.ion) {
            std::string ion_i_name = i_name + "_" + eff.ion.value();
            std::string ion_g_name = g_name + "_" + eff.ion.value();
            if (!effect_set.count(ion_i_name)) {
                // notice here that the source of i_ion is not only pointing to the current of the ion,
                // but also the overall current. This is on purpose.
                pointer_map.insert({ion_i_name, prefix(ion_i_name)});
                pointer_map.insert({ion_i_name, prefix(i_name)});
                field_pack.effect_sources.emplace_back(ion_i_name, i_effect, eff.ion);
                effect_set.insert(ion_i_name);
            }
            if (!effect_set.count(ion_g_name)) {
                // notice here that the source of g_ion is not pointing to the conductance of the ion,
                // but the overall conductance. This is on purpose.
                pointer_map.insert({ion_g_name, prefix(g_name)});
                effect_set.insert(ion_g_name);
            }
        }
    }

    /**** Fill procedure_pack using state_field_decoder ****/
    for (const auto& c: m.parameters) {
        // Simplify and save the parameter declarations
        auto param_value = std::get<resolved_parameter>(*c).value;
        if (!std::get_if<resolved_int>(param_value.get()) &&
            !std::get_if<resolved_float>(param_value.get())) {
            auto opt = optimizer(simplify(c, {}));
            procedure_pack.assigned_parameters.push_back(opt.optimize());
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

    /**** Fill proc_write_var maps using pointer_map and state_field_decoder ****/
    fill_write_maps(state_field_decoder);

    /**** Fill proc_read_var maps using variable_to_source_map ****/
    fill_read_maps(param_set, state_set, bind_set);
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
                    if (!pointer_map.count(field_mangled_name)) {
                        throw std::runtime_error(fmt::format("Internal compiler error: cannot find state {} with field {} "
                                                             "that is being initialized.", vname, field_name));
                    }
                    auto range = pointer_map.equal_range(field_mangled_name);
                    for (auto it = range.first; it != range.second; ++it) {
                        map.insert({it->second, field_value->name});
                    }
                }
                else {
                    if (!pointer_map.count(field_name)) {
                        throw std::runtime_error("Internal compiler error: can not find parameter that is being assigned.");
                    }
                    auto range = pointer_map.equal_range(field_name);
                    for (auto it = range.first; it != range.second; ++it) {
                        map.insert({it->second, field_value->name});
                    }
                }
            }
        }
        else {
            if (results.size() != 1) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected 1 result when writing {}.",
                                                     vname));
            }
            if (!pointer_map.count(vname)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being assigned.");
            }
            auto range = pointer_map.equal_range(vname);
            for (auto it = range.first; it != range.second; ++it) {
                map.insert({it->second, results.front().name});
            }
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

void printable_mechanism::fill_read_maps(
        std::unordered_set<std::string> param_set,
        std::unordered_set<std::string> state_set,
        std::unordered_set<std::string> bind_set)
{
    for (const auto& c: procedure_pack.assigned_parameters) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (pointer_map.count(a) != 1) {
                throw std::runtime_error("Internal compiler error: 0 or more than 1 sources found.");
            }
            auto range = pointer_map.equal_range(a);
            auto source_a = range.first->second;

            // Assigned parameters are parts of the initialization
            if (param_set.count(a)) {
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
            if (pointer_map.count(a) != 1) {
                throw std::runtime_error("Internal compiler error: 0 or more than 1 sources found.");
            }
            auto range = pointer_map.equal_range(a);
            auto source_a = range.first->second;

            if (param_set.count(a)) {
                init_read_map.parameter_map.insert({source_a, a});
            }
            else if (bind_set.count(a)) {
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
            if (pointer_map.count(a) != 1) {
                throw std::runtime_error("Internal compiler error: 0 or more than 1 sources found.");
            }
            auto range = pointer_map.equal_range(a);
            auto source_a = range.first->second;

            if (param_set.count(a)) {
                evolve_read_map.parameter_map.insert({source_a, a});
            }
            else if (bind_set.count(a)) {
                evolve_read_map.binding_map.insert({source_a, a});
            }
            else if (state_set.count(a)) {
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
            if (pointer_map.count(a) != 1) {
                throw std::runtime_error("Internal compiler error: 0 or more than 1 sources found.");
            }
            auto range = pointer_map.equal_range(a);
            auto source_a = range.first->second;

            if (param_set.count(a)) {
                effect_read_map.parameter_map.insert({source_a, a});
            }
            else if (bind_set.count(a)) {
                effect_read_map.binding_map.insert({source_a, a});
            }
            else if (state_set.count(a)) {
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