#include <cassert>
#include <string>
#include <sstream>
#include <unordered_map>

#include <arblang/optimizer/optimizer.hpp>
#include <arblang/pre_printer/check_mechanism.hpp>
#include <arblang/pre_printer/get_read_arguments.hpp>
#include <arblang/pre_printer/simplify.hpp>
#include <arblang/pre_printer/printable_mechanism.hpp>
#include <arblang/resolver/resolved_expressions.hpp>
#include <arblang/solver/solve.hpp>

#include <fmt/core.h>

#include "../util/rexp_helpers.hpp"

namespace al {
namespace resolved_ir {

// TODO Add support for nested param/state records.
printable_mechanism::printable_mechanism(const resolved_mechanism& mech, std::string i_name, std::string g_name)
    : current_field_name_(std::move(i_name)), conductance_field_name_(std::move(g_name))
{
    mech_kind = mech.kind;
    mech_name = mech.name;

    // Check that the mechanism can be printed
    check(mech);

    // Collect all possible writable variables.
    std::vector<std::pair<std::string, r_type>> mech_writables;
    for (const auto& s: mech.states) {
        auto state = is_resolved_state(s).value();
        mech_writables.emplace_back(state.name, state.type);
    }
    for (const auto& e: mech.parameters) {
        auto param = is_resolved_parameter(e).value();
        mech_writables.emplace_back(param.name, param.type);
    }
    for (const auto& e: mech.effects) {
        auto effect = is_resolved_effect(e).value();
        mech_writables.emplace_back(effect_rec_name_, effect.type);
    }

    // Generate map from record.field to mangled record_field name
    // e.g.
    //   state s: {a: real, b:real};
    //     s.a -> _s_a
    //     s.b -> _s_b
    //   param p: {a: real, b:voltage};
    //     p.a -> _p_a
    //     p.b -> _p_b
    //   effect current_pair;
    //     effect::current     -> _effect_`i_name`_
    //     effect::conductance -> _effect_`g_name`_
    auto record_field_decoder = gen_record_field_map(mech_writables);

    /**** Fill pointer_map, writable_variables, field_pack and ionic_fields ****/
    // States
    write_map writable_variables;
    for (const auto& s: mech.states) {
        auto state = is_resolved_state(s).value();
        auto state_name = state.name;

        // State variables can be written
        auto state_rec = is_resolved_record_type(state.type);
        if (!state_rec) {
            storage_info state_storage = {prefix(state_name), storage_class::internal};
            writable_variables.insert({state_name, state_storage});
            pointer_map.insert({state_name, state_storage});
            field_pack.state_sources.push_back(state_name);
        }
        else {
            for (const auto&[field_name, field_type]: state_rec->fields) {
                auto state_field_name = record_field_decoder.at(state_name).at(field_name);
                storage_info state_field_storage = {prefix(state_field_name), storage_class::internal};
                writable_variables.insert({state_field_name, state_field_storage});
                pointer_map.insert({state_field_name, state_field_storage});
                field_pack.state_sources.push_back(state_field_name);
            }
        }
    }

    // Parameters
    for (const auto& c: mech.parameters) {
        auto param = is_resolved_parameter(c).value();
        auto param_name = param.name;

        // Parameters can be written (if they are a function of other parameters, i.e. not constant).
        auto param_rec = is_resolved_record_type(param.type);
        if (!param_rec) {
            auto val = is_number(param.value);
            auto type = is_resolved_quantity_type(param.type)->type;
            field_pack.param_sources.emplace_back(param.name, val.value_or(NAN), to_string(type));
            storage_info storage = {prefix(param.name), storage_class::internal};
            pointer_map.insert({param.name, storage});
            if (!val) {
                writable_variables.insert({param.name, storage});
            }
        }
        else {
            auto param_obj = is_resolved_object(param.value);
            if (!param_obj) {
                throw std::runtime_error(fmt::format("Internal compiler error: Expected a resolved_object value "
                                                     "for parameter {}", param.name));
            }
            auto field_names  = param_obj->field_names();
            auto field_values = param_obj->field_values();
            assert(field_names.size() == field_values.size());

            std::unordered_map<std::string, std::optional<double>> param_map;
            for (unsigned i = 0; i < field_names.size(); ++i) {
                param_map.insert({field_names[i], is_number(field_values[i])});
            }
            for (const auto&[field_name, field_type]: param_rec->fields) {
                auto param_field_name = record_field_decoder.at(param_name).at(field_name);
                auto type = is_resolved_quantity_type(field_type)->type;
                auto field_val = param_map.at(field_name);
                field_pack.param_sources.emplace_back(param_field_name, field_val.value_or(NAN), to_string(type));
                storage_info field_storage = {prefix(param_field_name), storage_class::internal};
                pointer_map.insert({param_field_name, field_storage});
                if (!field_val) {
                    writable_variables.insert({param_field_name, field_storage});
                }
            }
        };
    }

    // Bindings
    std::unordered_map<std::string, unsigned> ion_idx;
    for (const auto& c: mech.bindings) {
        auto bind = is_resolved_bind(c).value();
        auto bind_name = bind.name;
        if (bind.ion) bind_name += ("_" + bind.ion.value());

        std::optional<double> scale;
        switch (bind.bind) {
            case bindable::temperature:            // input K -> K
            case bindable::current_density:        // input A/m^2 -> A/m^2
            case bindable::charge:                 // input real -> real
            case bindable::internal_concentration: // input mmol/L = mol/m^3 -> mol/m^3
            case bindable::external_concentration: // input mmol/L = mol/m^3 -> mol/m^3
            case bindable::nernst_potential:       // still unknown (not implemented)
            case bindable::molar_flux:             // still unknown (not implemented)
            case bindable::dt:                     // input ms -> ms (only usd in ode solution)
                break;
            case bindable::membrane_potential:     // input mV -> V
                scale = 1e-3; break;
                break;
            default: break;
        }

        // Bindables can not be written.
        auto storage = bind.ion? storage_class::ionic: storage_class::external;
        storage_info bind_storage = {prefix(bind_name), storage, bind.ion, scale};
        pointer_map.insert({bind_name, bind_storage});
        field_pack.bind_sources.emplace_back(bind_name, bind.bind, bind.ion);

        // Store ion info
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

    // Effects
    std::unordered_set<std::string> effect_set;
    auto append_effect = [&, this](const std::string& f_name, affectable f_effect, const std::optional<std::string>& f_ion) {
        if (!effect_set.count(f_name)) {
            field_pack.effect_sources.emplace_back(f_name, f_effect, f_ion);
            effect_set.insert(f_name);
        }
    };
    auto get_scale = [](affectable a) {
        std::optional<double> scale;
        switch (a) {
            case affectable::current_density:                   // A/m^2 -> arbor expects A/m^2
            case affectable::molar_flux:                        // still unknown (not implemented)
            case affectable::molar_flow_rate:                   // still unknown (not implemented)
            case affectable::internal_concentration_rate:       // still unknown (not implemented)
            case affectable::external_concentration_rate:       // still unknown (not implemented)
                break;
            case affectable::current:      scale = 1e9; break;  // A -> arbor expects nA
            case affectable::conductance:  scale = 1e6; break;  // S -> arbor expects uS
            case affectable::conductivity: scale = 1e-3; break; // S/m^2 -> arbor expects KS/m^2
            default: break;
        }
        return scale;
    };

    // Always add total current and conductance to writable_variables,
    // pointer_map and field_pack regardless of whether they are directly written.
    // Any contribution to an ionic current/conductance will also affect the
    // total current/conductance.
    auto mangled_i_name = mangle(effect_rec_name_, current_field_name_);
    auto mangled_g_name = mangle(effect_rec_name_, conductance_field_name_);

    affectable i_effect = affectable::current_density;
    affectable g_effect = affectable::conductivity;
    if (mech_kind == mechanism_kind::point || mech_kind == mechanism_kind::junction) {
        i_effect = affectable::current;
        g_effect = affectable::conductance;
    }

    storage_info total_i_storage = {prefix(mangled_i_name), storage_class::external, std::nullopt, get_scale(i_effect)};
    storage_info total_g_storage = {prefix(mangled_g_name), storage_class::external, std::nullopt, get_scale(g_effect)};

    {
        append_effect(mangled_i_name, i_effect, std::nullopt);
        pointer_map.insert({mangled_i_name, total_i_storage});
        writable_variables.insert({mangled_i_name, total_i_storage});

        append_effect(mangled_g_name, g_effect, std::nullopt);
        pointer_map.insert({mangled_g_name, total_g_storage});
        writable_variables.insert({mangled_g_name, total_g_storage});
    }

    // TODO handle effects other than current_denisty_pair and current_pair
    for (const auto& c: mech.effects) {
        auto eff = is_resolved_effect(c).value();
        if (eff.ion) {
            auto ion = eff.ion.value();
            if (!ion_idx.count(ion)) {
                ion_idx[ion] = ionic_fields.size();
                ionic_fields.push_back({ion, false, false, false});
            };

            auto ion_i_name = mangled_i_name + "_" + ion;
            auto ion_g_name = mangled_g_name + "_" + ion;

            append_effect(ion_i_name, i_effect, eff.ion);
            storage_info ion_i_storage = {prefix(ion_i_name), storage_class::ionic, ion, get_scale(i_effect)};

            pointer_map.insert({ion_i_name, ion_i_storage});

            // The ionic_current variable is written to the simulator's
            // ionic_current vector as well as the overall current vector.
            writable_variables.insert({ion_i_name, ion_i_storage});
            writable_variables.insert({ion_i_name, total_i_storage});

            // The ionic_conductance variable is written to the simulator's
            // ionic_conductance vector only.
            writable_variables.insert({ion_g_name, total_g_storage});
        }
    }

    /**** Fill procedure_pack after a final simplification of the mechanism methods ****/
    auto p_mech = simplify_mech(mech, record_field_decoder);
    for (const auto& c: p_mech.parameters) {
        auto param_value = is_resolved_parameter(c)->value;
        if (!is_trivial(param_value)) {
            procedure_pack.assigned_parameters.push_back(c);
        }
    }
    for (const auto& c: p_mech.effects) {
        procedure_pack.effects.push_back(c);
    }
    for (const auto& c: p_mech.initializations) {
        procedure_pack.initializations.push_back(c);
    }
    for (const auto& c: p_mech.on_events) {
        procedure_pack.on_events.push_back(c);
    }
    for (const auto& c: p_mech.evolutions) {
        procedure_pack.evolutions.push_back(c);
    }

    /**** Fill proc_write_var maps ****/
    fill_write_maps(record_field_decoder, writable_variables);

    /**** Fill proc_read_var maps using ****/
    fill_read_maps();
}

record_field_map printable_mechanism::gen_record_field_map(const std::vector<std::pair<std::string, r_type>>& writables) {
    // Collect the writable variables that have resolved_record type.
    // Create mangled names for the fields of the record.
    // e.g. state s: {m: real; h:real;} -> state _s_m:real and state _s_h:real
    record_field_map decoder;
    for (const auto& [name, type]: writables) {
        auto record = is_resolved_record_type(type);
        if (record) {
            std::unordered_map<std::string, std::string> rec_fields;
            for (const auto&[f_name, f_type]: record->fields) {
                decoder[name][f_name] = mangle(name, f_name);
            }
        }
    }

    // Add the overall current and conductance effects which need
    // to be present even if they are not directly written
    decoder[effect_rec_name_][current_field_name_] = mangle(effect_rec_name_, current_field_name_);
    decoder[effect_rec_name_][conductance_field_name_] =  mangle(effect_rec_name_, conductance_field_name_);

    return std::move(decoder);
}

void printable_mechanism::fill_write_maps(
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& record_field_decoder,
    const write_map& written_variables)
{
    auto form_result = [](const std::string& id, const r_expr& val) {
        // Get innermost result of val
        r_expr result = val;
        if (auto let_opt = is_resolved_let(val)) {
            result = get_innermost_body(&let_opt.value());
        }
        return resolved_variable(id, result, type_of(result), location_of(result));
    };

    auto write_var =
        [&](const resolved_variable& result,
            std::unordered_multimap<std::string, storage_info>& map)
   {
        auto val_to_string_visitor = al::util::overloaded {
                [&](const resolved_argument& t) {return t.name;},
                [&](const resolved_variable& t) {return t.name;},
                [&](const resolved_int& t)      {return std::to_string(t.value);},
                [&](const resolved_float& t)    {std::ostringstream os; os << t.value; return os.str();},
                [&](const auto& t) {return std::string();}
        };

        if (auto obj = is_resolved_object(result.value)) {
            // Only state variables can be objects because only they can have resolved_record type
            auto field_names  = obj->field_names();
            auto field_values = obj->field_values();
            assert(field_names.size() == field_values.size());

            for (unsigned i = 0; i < field_names.size(); ++i) {
                auto field_name = field_names[i];
                std::string val_string = std::visit(val_to_string_visitor, *field_values[i]);
                if (val_string.empty()) {
                    throw std::runtime_error(fmt::format("Internal compiler error: Expected a resolved_variable, "
                                                         "resolved_argument resolved_int or resolved_float and was "
                                                         "disappointed."));
                }
                if (!record_field_decoder.count(result.name) || !record_field_decoder.at(result.name).count(field_name)) {
                    throw std::runtime_error(fmt::format("Internal compiler error: cannot find variable {} with field {} "
                                                         "that is being written.", result.name, field_name));
                }
                auto field_mangled_name = record_field_decoder.at(result.name).at(field_name);
                if (!written_variables.count(field_mangled_name)) {
                    throw std::runtime_error(fmt::format("Internal compiler error: cannot find variable {} with field {} "
                                                         "that is being written.", result.name, field_name));
                }
                auto range = written_variables.equal_range(field_mangled_name);
                for (auto it = range.first; it != range.second; ++it) {
                    auto temp =  it->second;
                    map.insert({val_string, it->second});
                }
            }
        }
        else {
            std::string val_string = std::visit(val_to_string_visitor, *result.value);
            if (!written_variables.count(result.name)) {
                throw std::runtime_error(fmt::format("Internal compiler error: can not find parameter {} "
                                                     "that is being written.", result.name));
            }
            auto range = written_variables.equal_range(result.name);
            for (auto it = range.first; it != range.second; ++it) {
                auto temp =  it->second;
                map.insert({val_string, it->second});
            }
        }
    };

    // Fill the procedure-specific write maps.
    for (const auto& c: procedure_pack.initializations) {
        auto init = is_resolved_initial(c).value();

        auto state_name = is_resolved_argument(init.identifier)->name;
        auto state_assignment = form_result(state_name, init.value);
        write_var(state_assignment, init_write_map);
    }

    for (const auto& c: procedure_pack.on_events) {
        auto init = is_resolved_on_event(c).value();

        auto state_name = is_resolved_argument(init.identifier)->name;
        auto state_assignment = form_result(state_name, init.value);
        write_var(state_assignment, event_write_map);
    }

    for (const auto& c: procedure_pack.evolutions) {
        auto evolve = is_resolved_evolve(c).value();

        auto state_name = is_resolved_argument(evolve.identifier)->name;
        auto state_assignment  = form_result(state_name, evolve.value);
        write_var(state_assignment, evolve_write_map);
    }

    for (const auto& c: procedure_pack.effects) {
        auto effect = is_resolved_effect(c).value();

        auto effect_assignment = form_result(effect_rec_name_, effect.value);
        write_var(effect_assignment, effect_write_map);
    }

    for (const auto& c: procedure_pack.assigned_parameters) {
        auto param = is_resolved_parameter(c).value();

        auto param_name = param.name;
        auto param_assignment = form_result(param_name, param.value);
        write_var(param_assignment, init_write_map);
    }
}

void printable_mechanism::fill_read_maps() {
    for (const auto& c: procedure_pack.assigned_parameters) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (!pointer_map.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
            }
            auto source_a = pointer_map.at(a);

            // Assigned parameters are parts of the initialization
            init_read_map.insert({a, source_a});
        }
    }

    for (const auto& c: procedure_pack.initializations) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (!pointer_map.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
            }
            auto source_a = pointer_map.at(a);
            init_read_map.insert({a, source_a});
        }
    }

    for (const auto& c: procedure_pack.on_events) {
        auto event = is_resolved_on_event(c).value();
        auto arg   = is_resolved_argument(event.argument).value();

        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (a == arg.name) {
                // this is the weight member of the event stream variable
                event_read_map.insert({a, {"weight", storage_class::stream_member}});
            }
            else if (pointer_map.count(a)) {
                event_read_map.insert({a, pointer_map.at(a)});
            }
            else {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
            }
        }
    }

    for (const auto& c: procedure_pack.evolutions) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (!pointer_map.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
            }
            auto source_a = pointer_map.at(a);
            evolve_read_map.insert({a, source_a});
        }
    }

    for (const auto& c: procedure_pack.effects) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (!pointer_map.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
            }
            auto source_a = pointer_map.at(a);
            effect_read_map.insert({a, source_a});
        }
    }
}

resolved_mechanism printable_mechanism::simplify_mech(const resolved_mechanism& mech, const record_field_map& field_map) {
    resolved_mechanism s_mech;

    std::vector<r_expr> param_exprs;
    for (const auto& c: mech.parameters) {
        auto opt = optimizer(simplify(c, {}));
        param_exprs.push_back(opt.optimize());
    }
    for (const auto& c: mech.effects) {
        auto opt = optimizer(simplify(c, field_map));
        s_mech.effects.push_back(opt.optimize());
    }
    for (const auto& c: mech.initializations) {
        auto opt = optimizer(simplify(c, field_map));
        s_mech.initializations.push_back(opt.optimize());
    }
    for (const auto& c: mech.on_events) {
        auto opt = optimizer(simplify(c, field_map));
        s_mech.on_events.push_back(opt.optimize());
    }
    for (const auto& c: mech.evolutions) {
        auto opt = optimizer(simplify(c, field_map));
        s_mech.evolutions.push_back(opt.optimize());
    }

    // If a parameter reads from another parameter:
    // e.g.
    // parameter a = 4;
    // parameter b = a + 4;
    // The parameter assignments will both end up in the body of the init() function.
    // `a` is a resolved_argument in the second case, which will typically get read
    // from memory.
    // To avoid that, we can propagate the actual value of `a` to `b`.

    std::unordered_map<std::string, r_expr> param_map;
    for (const auto& c: param_exprs) {
        s_mech.parameters.push_back(copy_propagate(c, param_map).first);
        auto param = is_resolved_parameter(c).value();
        if (!is_trivial(param.value)) {
            r_expr result = param.value;
            if (auto let_opt = is_resolved_let(param.value)) {
                result = get_innermost_body(&let_opt.value());
            }
            param_map.insert({param.name, result});
        }
    }
    s_mech.name = mech.name;
    s_mech.kind = mech.kind;
    return s_mech;
}

} // namespace resolved_ir
} // namespace al