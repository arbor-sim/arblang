#include <unordered_set>

#include "fmt/core.h"

#include "arblang/pre_printer/check_mechanism.hpp"

namespace al {
namespace resolved_ir {

// Check that the mechanism is printable.
// Should throw an exception for:
// 1. All unsupported features.
// 2. Any leftover user errors.
// 3. And compiler errors.

struct mech_error: std::runtime_error {
    mech_error(const std::string& what_arg): std::runtime_error("Error in pre-printer check: " + what_arg) {}
};

void check(const resolved_mechanism& e) {
    if (e.kind != mechanism_kind::density && e.kind != mechanism_kind::point) {
        throw mech_error(fmt::format("Unsupported mechanism kind {} for mechanism {}, still a work in progress.",
                                     to_string(e.kind), e.name));
    }
    if (e.kind != mechanism_kind::point && !e.on_events.empty()) {
        throw mech_error(fmt::format("Unsupported API call `on_events` for mechanism kind {} (mechanism {}).",
                                     to_string(e.kind), e.name));
    }
    if (!e.functions.empty()) {
        throw mech_error(fmt::format("Internal compiler error, expected zero functions after inlining."));
    }
    if (!e.constants.empty()) {
        throw mech_error(fmt::format("Internal compiler error, expected zero constants after constant propagation."));
    }

    for (const auto& a: e.states) {
        auto p = is_resolved_state(a);
        if (!p) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_state in "
                                         "resolved_mechanism::states"));
        }
        auto t = is_resolved_record_type(p->type);
        if (t) {
            for (const auto& [f_id, f_type]: t->fields) {
                auto ft = is_resolved_record_type(f_type);
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
        auto p = is_resolved_parameter(a);
        if (!p) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_parameter in "
                                         "resolved_mechanism::parameters"));
        }
        auto t = is_resolved_record_type(p->type);
        if (t) {
            for (const auto& [f_id, f_type]: t->fields) {
                auto ft = is_resolved_record_type(f_type);
                if (ft) {
                    throw mech_error(fmt::format("Unsupported nested records for parameters, still a work in progress",
                                                 to_string(e.kind), e.name));
                }
            }
        }
        if (is_resolved_int(p->value) || is_resolved_float(p->value)) {
            const_params.insert(p->name);
        }
        else {
            assigned_params.insert(p->name);
        }
    }

    for (const auto& a: e.exports) {
        auto x = is_resolved_export(a);
        if (!x) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_export in "
                                         "resolved_mechanism::exports"));
        }
        auto p = is_resolved_argument(x->identifier);
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

    for (const auto& a: e.bindings) {
        auto b = is_resolved_bind(a);
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
        auto b = is_resolved_effect(a);
        if (!b) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_effect in "
                                         "resolved_mechanism::effects"));
        }
        auto effect = b->effect;
        switch (effect) {
            case affectable::molar_flux: {
                if (e.kind != mechanism_kind::density) {
                    throw mech_error(fmt::format("User error: unsupported effect {} for mechanism kind {} at {}",
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
                auto rec = is_resolved_record_type(b->type);
                if (!rec) {
                    throw mech_error(fmt::format("Internal compiler error, expected affectable {} to have been "
                                                 "resolved_record type.", to_string(effect)));
                }
            }
            default: break;
        }
    }

    for (const auto& a: e.initializations) {
        auto init = is_resolved_initial(a);
        if (!init) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_initial in "
                                         "resolved_mechanism::initializations"));
        }
        auto arg = is_resolved_argument(init->identifier);
        if (!arg) {
            throw mech_error("Internal compiler error: expected identifier of resolved_initial to be a "
                             "resolved_argument.");
        }
    }
    for (const auto& a: e.on_events) {
        auto on_event = is_resolved_on_event(a);
        if (!on_event) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_on_event in "
                                         "resolved_mechanism::on_events"));
        }
        auto arg = is_resolved_argument(on_event->argument);
        if (!arg) {
            throw mech_error("Internal compiler error: expected argument of resolved_on_event to be a "
                             "resolved_argument.");
        }
        auto iden = is_resolved_argument(on_event->identifier);
        if (!iden) {
            throw mech_error("Internal compiler error: expected identifier of resolved_on_event to be a "
                             "resolved_argument.");
        }
    }
    for (const auto& a: e.evolutions) {
        auto evolve = is_resolved_evolve(a);
        if (!evolve) {
            throw mech_error(fmt::format("Internal compiler error, expected resolved_evolve in "
                                         "resolved_mechanism::evolutions"));
        }
        auto arg = is_resolved_argument(evolve->identifier);
        if (!arg) {
            throw mech_error("Internal compiler error: expected identifier of resolved_evolve to be a "
                             "resolved_argument.");
        }
    }
}

} // namespace resolved_ir
} // namespace al