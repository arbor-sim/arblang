#include <unordered_set>

#include "fmt/core.h"

#include "arblang/pre_printer/check_mechanism.hpp"

namespace al {
namespace resolved_ir {

// Check that the mechanism is printable
// Should throw an exception for all unsupported features,
// Any leftover user errors,
// And any compiler errors.

struct mech_error: std::runtime_error {
    mech_error(const std::string& what_arg): std::runtime_error("Error in pre-printer check: " + what_arg) {}
};

void check(const resolved_mechanism& e) {
    if (e.kind != mechanism_kind::density && e.kind != mechanism_kind::point) {
        throw mech_error(fmt::format("Unsupported mechanism kind {} for mechanism {}, still a work in progress",
                                     to_string(e.kind), e.name));
    }
    if (e.kind != mechanism_kind::point && !e.on_events.empty()) {
        throw mech_error(fmt::format("Unsupported API call `on_events` for mechanism kind {} (mechanism {})",
                                     to_string(e.kind), e.name));
    }
    if (!e.functions.empty()) {
        throw mech_error(fmt::format("Internal compiler error, expected zero functions after inlining"));
    }
    if (!e.constants.empty()) {
        throw mech_error(fmt::format("Internal compiler error, expected zero constants after constant propagation"));
    }

    for (const auto& a: e.states) {
        auto p = std::get_if<resolved_state>(a.get());
        if (!p) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_state in "
                                         "resolved_mechanism::states"));
        }
        auto t = std::get_if<resolved_record>(p->type.get());
        if (t) {
            for (const auto& [f_id, f_type]: t->fields) {
                auto ft = std::get_if<resolved_record>(f_type.get());
                if (ft) {
                    throw mech_error(fmt::format("Unsupported nested records for states, still a work in progress",
                                                 to_string(e.kind), e.name));
                }
            }
        }
    }

    std::unordered_set<std::string> const_params;
    std::unordered_set<std::string> assigned_params;
    for (const auto& a: e.parameters) {
        auto p = std::get_if<resolved_parameter>(a.get());
        if (!p) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_parameter in "
                                         "resolved_mechanism::parameters"));
        }
        auto t = std::get_if<resolved_record>(p->type.get());
        if (t) {
            for (const auto& [f_id, f_type]: t->fields) {
                auto ft = std::get_if<resolved_record>(f_type.get());
                if (ft) {
                    throw mech_error(fmt::format("Unsupported nested records for parameters, still a work in progress",
                                                 to_string(e.kind), e.name));
                }
            }
        }
        if (std::get_if<resolved_int>(p->value.get()) || std::get_if<resolved_float>(p->value.get())) {
            const_params.insert(p->name);
        }
        else {
            assigned_params.insert(p->name);
        }
    }

    for (const auto& a: e.exports) {
        auto x = std::get_if<resolved_export>(a.get());
        if (!x) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_export in "
                                         "resolved_mechanism::exports"));
        }
        auto p = std::get_if<resolved_argument>(x->identifier.get());
        if (!p) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_argument as the identifier "
                                         "of a resolved_export"));
        }

        std::string x_name = p->name;
        if (assigned_params.count(x_name)) {
            throw mech_error(fmt::format("User error: cannot export {} because it's value is based on another parameter.",
                                         x_name));
        }
        if (!const_params.count(x_name)) {
            throw mech_error(fmt::format("Internal compiler error, cannot export parameter {} because it was "
                                         "not found or it was exported twice.", x_name));
        }
        const_params.erase(x_name);
    }
    if (!const_params.empty()) {
        throw mech_error(fmt::format("Internal compiler error, expected parameter {} to have been constant "
                                     "propagated if not exported", *const_params.begin()));
    }

    //membrane_potential, temperature, current_density,
    //    molar_flux, charge,
    //    internal_concentration, external_concentration, nernst_potential,
    //    dt

    for (const auto& a: e.bindings) {
        auto b = std::get_if<resolved_bind>(a.get());
        if (!b) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_bind in "
                                         "resolved_mechanism::bindings"));
        }
        if ((e.kind != mechanism_kind::concentration) &&
            ((b->bind == bindable::molar_flux) || (b->bind == bindable::current_density)))
        {
            throw mech_error(fmt::format("User error: unsupported bindable {} for mechanism kind {} at {}",
                                         to_string(b->bind), to_string(e.kind), to_string(e.loc)));
        }
        if (b->bind == bindable::molar_flux || b->bind == bindable::nernst_potential) {
            // TODO Add support for molar_flux and nernst potential
            throw mech_error(fmt::format("Unsupported bindable {} at {}, still a work in progress.",
                                         to_string(b->bind), to_string(e.loc)));
        }
    }

    for (const auto& a: e.effects) {
        auto b = std::get_if<resolved_effect>(a.get());
        if (!b) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_effect in "
                                         "resolved_mechanism::effects"));
        }
        auto effect = b->effect;
        switch (effect) {
            case affectable::molar_flux: {
                if (e.kind != mechanism_kind::density) {
                    throw mech_error(fmt::format("User errror: unsupported effect {} for mechanism kind {} at {}",
                                                 to_string(effect), to_string(e.kind), to_string(e.loc)));
                }
                // TODO Add support for molar_flux
                throw mech_error(fmt::format("Unsupported effect {} at {}, still a work in progress.",
                                                     to_string(effect), to_string(e.loc)));
            }
            case affectable::molar_flow_rate: {
                if (e.kind != mechanism_kind::point) {
                    throw mech_error(fmt::format("User error: unsupported effect {} for mechanism kind {} at {}",
                                                 to_string(effect), to_string(e.kind), to_string(e.loc)));
                }
                // TODO Add support for molar_flow_rate
                throw mech_error(fmt::format("Unsupported effect {} at {}, still a work in progress.",
                                                     to_string(effect), to_string(e.loc)));
            }
            case affectable::internal_concentration_rate:
            case affectable::external_concentration_rate: {
                if (e.kind != mechanism_kind::concentration) {
                    throw mech_error(fmt::format("User error: unsupported effect {} for mechanism kind {} at {}",
                                                 to_string(effect), to_string(e.kind), to_string(e.loc)));
                }
                // TODO Add support for internal/external_concentration_rate
                throw mech_error(fmt::format("Unsupported effect {} at {}, still a work in progress.",
                                             to_string(effect), to_string(e.loc)));
            }
            case affectable::current_density:
            case affectable::current: {
                throw mech_error("Internal compiler error: Unexpected current/current_density "
                                 "affectable at this stage of the compilation. Instead expected "
                                 "current_density_pair or current_pair");
            }
            case affectable::conductivity:
            case affectable::conductance: {
                throw mech_error("Internal compiler error: Unexpected conductance/conductivity "
                                 "affectable at this stage of the compilation. Instead expected "
                                 "current_density_pair or current_pair");
            }
            case affectable::current_density_pair:
            case affectable::current_pair: {
                auto rec = std::get_if<resolved_record>(b->type.get());
                if (!rec) {
                    throw mech_error(fmt::format("Internal compiler error, expected affectable {} to have been "
                                                 "resolved_record type.", to_string(effect)));
                }
            }
            default: break;
        }
    }

    for (const auto& a: e.initializations) {
        auto init = std::get_if<resolved_initial>(a.get());
        if (!init) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_initial in "
                                         "resolved_mechanism::initializations"));
        }
        auto arg = std::get_if<resolved_argument>(init->identifier.get());
        if (!arg) {
            throw mech_error("Internal compiler error: expected identifier of resolved_initial to be a "
                             "resolved_argument.");
        }
    }
    for (const auto& a: e.on_events) {
        auto on_event = std::get_if<resolved_on_event>(a.get());
        if (!on_event) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_on_event in "
                                         "resolved_mechanism::on_events"));
        }
        auto arg = std::get_if<resolved_argument>(on_event->argument.get());
        if (!arg) {
            throw mech_error("Internal compiler error: expected argument of resolved_on_event to be a "
                             "resolved_argument.");
        }
        auto iden = std::get_if<resolved_argument>(on_event->identifier.get());
        if (!iden) {
            throw mech_error("Internal compiler error: expected identifier of resolved_on_event to be a "
                             "resolved_argument.");
        }
    }
    for (const auto& a: e.evolutions) {
        auto evolve = std::get_if<resolved_evolve>(a.get());
        if (!evolve) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_evolve in "
                                         "resolved_mechanism::evolutions"));
        }
        auto arg = std::get_if<resolved_argument>(evolve->identifier.get());
        if (!arg) {
            throw mech_error("Internal compiler error: expected identifier of resolved_evolve to be a "
                             "resolved_argument.");
        }
    }
}

} // namespace resolved_ir
} // namespace al