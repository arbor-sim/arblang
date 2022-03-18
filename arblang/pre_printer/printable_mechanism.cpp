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

// TODO Add support for
//- Param records.
//- Nested param/state records.
printable_mechanism::printable_mechanism(const resolved_mechanism& mech, std::string i_name, std::string g_name)
    : current_field_name_(std::move(i_name)), conductance_field_name_(std::move(g_name))
{
    mech_kind = mech.kind;
    mech_name = mech.name;

    // Check that the mechanism can be printed
    check(mech);

    // Generate map from record.field to mangled record_field name
    std::vector<std::pair<std::string, r_type>> mech_writables;
    for (const auto& s: mech.states) {
        auto state = std::get<resolved_state>(*s);
        mech_writables.emplace_back(state.name, state.type);
    }
    for (const auto& e: mech.parameters) {
        auto param = std::get<resolved_parameter>(*e);
        mech_writables.emplace_back(param.name, param.type);
    }
    for (const auto& e: mech.effects) {
        auto effect = std::get<resolved_effect>(*e);
        mech_writables.emplace_back(effect_rec_name_, effect.type);
    }
    auto record_field_decoder = gen_record_field_map(mech_writables);

    /**** Fill source_pointer_map, dest_pointer_map and field_pack ****/
    // States
    for (const auto& s: mech.states) {
        auto state = std::get<resolved_state>(*s);
        auto state_name = state.name;

        auto state_rec = std::get_if<resolved_record>(state.type.get());
        if (!state_rec) {
            source_pointer_map.insert({state_name, {prefix(state_name), storage_class::internal}});
            dest_pointer_map.insert({state_name, {prefix(state_name), storage_class::internal}});
            field_pack.state_sources.push_back(state_name);
        }
        else {
            for (const auto&[field_name, field_type]: state_rec->fields) {
                auto state_field_name = record_field_decoder.at(state_name).at(field_name);
                source_pointer_map.insert({state_field_name, {prefix(state_field_name), storage_class::internal}});
                dest_pointer_map.insert({state_field_name, {prefix(state_field_name), storage_class::internal}});
                field_pack.state_sources.push_back(state_field_name);
            }
        }
    }

    // Parameters
    for (const auto& c: mech.parameters) {
        auto param = std::get<resolved_parameter>(*c);
        auto param_name = param.name;
        auto trivial = is_trivial(param.value);

        auto param_rec = std::get_if<resolved_record>(param.type.get());
        if (!param_rec) {
            auto val = as_number(param.value);
            auto type = std::get<resolved_quantity>(*param.type).type;
            field_pack.param_sources.emplace_back(param.name, val.value_or(NAN), to_string(type));
            source_pointer_map.insert({param.name, {prefix(param.name), storage_class::internal}});
            if (!trivial) {
                dest_pointer_map.insert({param.name, {prefix(param.name), storage_class::internal}});
            }
        }
        else {
            std::unordered_map<std::string, double> param_map;
            auto param_obj = std::get_if<resolved_object>(param.value.get());
            if (param_obj) {
                auto field_names  = param_obj->field_names();
                auto field_values = param_obj->field_values();
                assert(field_names.size() == field_values.size());

                for (unsigned i = 0; i < field_names.size(); ++i) {
                    param_map.insert({field_names[i], as_number(field_values[i]).value_or(NAN)});
                }
            }
            for (const auto&[field_name, field_type]: param_rec->fields) {
                auto param_field_name = record_field_decoder.at(param_name).at(field_name);
                auto type = std::get<resolved_quantity>(*field_type).type;
                if (!param_map.count(field_name)) {
                    field_pack.param_sources.emplace_back(param_field_name, NAN, to_string(type));
                }
                else {
                    field_pack.param_sources.emplace_back(param_field_name, param_map.at(field_name), to_string(type));
                }
                source_pointer_map.insert({param_field_name, {prefix(param_field_name), storage_class::internal}});
                if (!trivial) {
                    dest_pointer_map.insert({param_field_name, {prefix(param_field_name), storage_class::internal}});
                }
            }

        };

    }

    // Bindings
    std::unordered_map<std::string, unsigned> ion_idx;
    for (const auto& c: mech.bindings) {
        auto bind = std::get<resolved_bind>(*c);
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

        auto storage = bind.ion? storage_class::ionic: storage_class::external;
        source_pointer_map.insert({bind_name, {prefix(bind_name), storage, bind.ion, scale}});
        field_pack.bind_sources.emplace_back(bind_name, bind.bind, bind.ion);

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
    // Always add to the destination and source maps and field pack the info
    // about the overall current and conductance regardless of whether or not
    // they are written. (Similar to record_field_decoder)
    auto mangled_i_name = mangle(effect_rec_name_, current_field_name_);
    auto mangled_g_name = mangle(effect_rec_name_, conductance_field_name_);
    {
        auto i_effect = (mech_kind == mechanism_kind::point || mech_kind == mechanism_kind::junction)?
                affectable::current: affectable::current_density;
        dest_pointer_map.insert({mangled_i_name, {prefix(mangled_i_name), storage_class::external, std::nullopt, get_scale(i_effect)}});
        source_pointer_map.insert({mangled_i_name, {prefix(mangled_i_name), storage_class::external, std::nullopt, get_scale(i_effect)}});
        append_effect(mangled_i_name, i_effect, std::nullopt);

        auto g_effect = (mech_kind == mechanism_kind::point || mech_kind == mechanism_kind::junction)?
                        affectable::conductance: affectable::conductivity;
        dest_pointer_map.insert({mangled_g_name, {prefix(mangled_g_name), storage_class::external, std::nullopt, get_scale(g_effect)}});
        source_pointer_map.insert({mangled_g_name, {prefix(mangled_g_name), storage_class::external, std::nullopt, get_scale(g_effect)}});
        append_effect(mangled_g_name, g_effect, std::nullopt);
    }
    for (const auto& c: mech.effects) {
        // TODO handle effects other than current_denisty_pair and current_pair
        auto eff = std::get<resolved_effect>(*c);
        if (eff.ion) {
            auto i_effect = eff.effect == affectable::current_density_pair?
                    affectable::current_density: affectable::current;
            auto g_effect = eff.effect == affectable::current_density_pair?
                    affectable::conductivity: affectable::conductance;

            auto i_scale = get_scale(i_effect);
            auto g_scale = get_scale(g_effect);

            auto ion = eff.ion.value();
            if (!ion_idx.count(ion)) {
                ion_idx[ion] = ionic_fields.size();
                ionic_fields.push_back({ion, false, false, false});
            };

            auto ion_i_name = mangled_i_name + "_" + ion;
            auto ion_g_name = mangled_g_name + "_" + ion;

            // destination of the ionic_current is the ionic_current and the overall current
            dest_pointer_map.insert({ion_i_name, {prefix(ion_i_name), storage_class::ionic, ion, i_scale}});
            dest_pointer_map.insert({ion_i_name, {prefix(mangled_i_name), storage_class::external, std::nullopt, i_scale}});

            // source of the ionic_current is the ionic_current only
            source_pointer_map.insert({ion_i_name, {prefix(ion_i_name), storage_class::ionic, ion, i_scale}});
            append_effect(ion_i_name, i_effect, eff.ion);

            // destination of the ionic_conductance is the ionic_conductance only. We never need to read it, so
            // no source is added.
            dest_pointer_map.insert({ion_g_name, {prefix(mangled_g_name), storage_class::external, std::nullopt, g_scale}});
        }
    }

    /**** Fill procedure_pack after a final simplification of the mechanism methods ****/
    auto p_mech = simplify_mech(mech, record_field_decoder);
    for (const auto& c: p_mech.parameters) {
        auto param_value = std::get<resolved_parameter>(*c).value;
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

    /**** Fill proc_write_var maps using pointer_map and record_field_decoder ****/
    fill_write_maps(record_field_decoder);

    /**** Fill proc_read_var maps using variable_to_source_map ****/
    fill_read_maps();
}

record_field_map printable_mechanism::gen_record_field_map(const std::vector<std::pair<std::string, r_type>>& writables) {
    // Collect the writable variables that have resolved_record type.
    // Create mangled names for the fields of the record.
    // e.g. state s: {m: real; h:real;} -> state _s_m:real and state _s_h:real
    record_field_map decoder;
    for (const auto& [name, type]: writables) {
        auto record = std::get_if<resolved_record>(type.get());
        if (record) {
            std::unordered_map<std::string, std::string> rec_fields;
            for (const auto&[f_name, f_type]: record->fields) {
                std::string mangled_name = mangle(name, f_name);
                decoder[name][f_name] = mangled_name;
            }
        }
    }

    // Add the overall current and conductance effects which need
    // to be present even if they are not directly written
    {
        std::unordered_map<std::string, std::string> rec_fields = {
            {current_field_name_, mangle(effect_rec_name_, current_field_name_)},
            {conductance_field_name_, mangle(effect_rec_name_, conductance_field_name_)}
        };
        decoder.insert({effect_rec_name_, std::move(rec_fields)});
    }
    return std::move(decoder);
}

void printable_mechanism::fill_write_maps(
    const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& record_field_decoder)
{
    auto form_result = [](const std::string& id, const r_expr& val) {
        // Get innermost result of val
        r_expr result = val;
        if (auto let_opt = get_let(val)) {
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

        if (auto obj = std::get_if<resolved_object>(result.value.get())) {
            // Only state variables can be set to objects because only they can have resolved_record type
            // TODO, allow parameter variables to have resolved_record type
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
                                                         "that is being initialized.", result.name, field_name));
                }
                auto field_mangled_name = record_field_decoder.at(result.name).at(field_name);
                if (!dest_pointer_map.count(field_mangled_name)) {
                    throw std::runtime_error(fmt::format("Internal compiler error: cannot find variable {} with field {} "
                                                         "that is being initialized.", result.name, field_name));
                }
                auto range = dest_pointer_map.equal_range(field_mangled_name);
                for (auto it = range.first; it != range.second; ++it) {
                    auto temp =  it->second;
                    map.insert({val_string, it->second});
                }
            }
        }
        else {
            std::string val_string = std::visit(val_to_string_visitor, *result.value);
            if (!dest_pointer_map.count(result.name)) {
                throw std::runtime_error(fmt::format("Internal compiler error: can not find parameter {} "
                                                     "that is being assigned.", result.name));
            }
            auto range = dest_pointer_map.equal_range(result.name);
            for (auto it = range.first; it != range.second; ++it) {
                auto temp =  it->second;
                map.insert({val_string, it->second});
            }
        }
    };

    for (const auto& c: procedure_pack.initializations) {
        auto init = std::get<resolved_initial>(*c);

        auto state_name = std::get<resolved_argument>(*init.identifier).name;
        auto state_assignment = form_result(state_name, init.value);
        write_var(state_assignment, init_write_map);
    }

    for (const auto& c: procedure_pack.on_events) {
        auto init = std::get<resolved_on_event>(*c);

        auto state_name = std::get<resolved_argument>(*init.identifier).name;
        auto state_assignment = form_result(state_name, init.value);
        write_var(state_assignment, event_write_map);
    }

    for (const auto& c: procedure_pack.evolutions) {
        auto evolve = std::get<resolved_evolve>(*c);

        auto state_name = std::get<resolved_argument>(*evolve.identifier).name;
        auto state_assignment  = form_result(state_name, evolve.value);
        write_var(state_assignment, evolve_write_map);
    }

    for (const auto& c: procedure_pack.effects) {
        auto effect = std::get<resolved_effect>(*c);

        auto effect_assignment = form_result(effect_rec_name_, effect.value);
        write_var(effect_assignment, effect_write_map);
    }

    for (const auto& c: procedure_pack.assigned_parameters) {
        auto param = std::get<resolved_parameter>(*c);

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
            if (!source_pointer_map.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
            }
            auto source_a = source_pointer_map.at(a);

            // Assigned parameters are parts of the initialization
            init_read_map.insert({a, source_a});
        }
    }

    for (const auto& c: procedure_pack.initializations) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (!source_pointer_map.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
            }
            auto source_a = source_pointer_map.at(a);
            init_read_map.insert({a, source_a});
        }
    }

    for (const auto& c: procedure_pack.on_events) {
        auto event = std::get<resolved_on_event>(*c);
        auto arg   = std::get<resolved_argument>(*event.argument);

        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (a == arg.name) {
                // this is the weight member of the event stream variable
                event_read_map.insert({a, {"weight", storage_class::stream_member}});
            }
            else if (source_pointer_map.count(a)) {
                event_read_map.insert({a, source_pointer_map.at(a)});
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
            if (!source_pointer_map.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
            }
            auto source_a = source_pointer_map.at(a);
            evolve_read_map.insert({a, source_a});
        }
    }

    for (const auto& c: procedure_pack.effects) {
        std::vector<std::string> read_args;
        read_arguments(c, read_args);

        for (const auto& a: read_args) {
            if (!source_pointer_map.count(a)) {
                throw std::runtime_error("Internal compiler error: can not find parameter that is being read.");
            }
            auto source_a = source_pointer_map.at(a);
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
        // Simplify and save the initializations
        auto opt = optimizer(simplify(c, field_map));
        s_mech.initializations.push_back(opt.optimize());
    }
    for (const auto& c: mech.on_events) {
        // Simplify and save the on_events
        auto opt = optimizer(simplify(c, field_map));
        s_mech.on_events.push_back(opt.optimize());
    }
    for (const auto& c: mech.evolutions) {
        // Simplify and save the evolutions
        auto opt = optimizer(simplify(c, field_map));
        s_mech.evolutions.push_back(opt.optimize());
    }

    // If a parameter reads from another parameter:
    // e.g.
    // parameter a = 4;
    // parameter b = a + 4;
    // The parameter assignments will both end up in the body of the init() function.
    // `a` is a resolved_argument in the second case, which will get read from memory.
    // To avoid that, we can progagate the actual value of `a` to `b`.

    std::unordered_map<std::string, r_expr> param_map;
    for (const auto& c: param_exprs) {
        s_mech.parameters.push_back(copy_propagate(c, param_map).first);
        auto param = std::get<resolved_parameter>(*c);
        if (!is_trivial(param.value)) {
            r_expr result = param.value;
            if (auto let_opt = get_let(param.value)) {
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