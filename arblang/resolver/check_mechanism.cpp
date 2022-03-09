#include <unordered_set>

#include <fmt/core.h>

#include <arblang/resolver/check_mechanism.hpp>

namespace al {
namespace resolved_ir {

// Check mechanism
void check(const resolved_mechanism& e) {
    std::unordered_set<std::string> const_params;
    std::unordered_set<std::string> assigned_params;
    for (const auto& a: e.parameters) {
        auto p = std::get<resolved_parameter>(*a);

        if (std::get_if<resolved_int>(p.value.get()) || std::get_if<resolved_float>(p.value.get())) {
            const_params.insert(p.name);
        }
        else {
            assigned_params.insert(p.name);
        }
    }

    for (const auto& a: e.exports) {
        auto x = std::get<resolved_export>(*a);
        std::string x_name;
        if (auto p = std::get_if<resolved_argument>(x.identifier.get())) {
            x_name = p->name;
        }
        else {
            throw std::runtime_error(fmt::format("Internal compiler error, expected resolved_argument instead of {} at {}",
                                                 to_string(x.identifier), to_string(x.loc)));
        }
        if (assigned_params.count(x_name)) {
            throw std::runtime_error(fmt::format("Cannot export {} because it's value is based on another parameter.",
                                                 x_name));
        }
        if (!const_params.count(x_name)) {
            throw std::runtime_error(fmt::format("Internal compiler error, cannot export parameter {} because it was not found",
                                                 x_name));
        }
        const_params.erase(x_name);
    }

    if (!const_params.empty()) {
        throw std::runtime_error(fmt::format("Internal compiler error, expected parameter {} to have been constant "
                                             "propagated if not exported", *const_params.begin()));
    }

    for (const auto& a: e.bindings) {
        auto b = std::get<resolved_bind>(*a);
        if ((e.kind != mechanism_kind::concentration) &&
            ((b.bind == bindable::molar_flux) || (b.bind == bindable::current_density)))
        {
            throw std::runtime_error(fmt::format("Unsupported bindable {} for mechanism kind {} at {}",
                                                 to_string(b.bind), to_string(e.kind), to_string(e.loc)));
        }
    }
    for (const auto& a: e.effects) {
        auto b = std::get<resolved_effect>(*a);
        switch (b.effect) {
            case affectable::current_density:
            case affectable::molar_flux: {
                if (e.kind != mechanism_kind::density) {
                    throw std::runtime_error(fmt::format("Unsupported effect {} for mechanism kind {} at {}",
                                                         to_string(b.effect), to_string(e.kind), to_string(e.loc)));
                }
                break;
            }
            case affectable::current:
            case affectable::molar_flow_rate: {
                if (e.kind != mechanism_kind::point) {
                    throw std::runtime_error(fmt::format("Unsupported effect {} for mechanism kind {} at {}",
                                                         to_string(b.effect), to_string(e.kind), to_string(e.loc)));
                }
                break;
            }
            case affectable::internal_concentration_rate:
            case affectable::external_concentration_rate: {
                if (e.kind != mechanism_kind::concentration) {
                    throw std::runtime_error(fmt::format("Unsupported effect {} for mechanism kind {} at {}",
                                                         to_string(b.effect), to_string(e.kind), to_string(e.loc)));
                }
                break;
            }
        }
    }

    // Only for now, throw an error if we are trying to effect anythin other than current_densty and current
    for (const auto& a: e.effects) {
        auto b = std::get<resolved_effect>(*a);
        if (b.effect != affectable::current_density && b.effect != affectable::current) {
            throw std::runtime_error(fmt::format("Unsupported effect {} at {}, still a work in progress.",
                                                 to_string(b.effect), to_string(e.loc)));
        }
    }
}

} // namespace resolved_ir
} // namespace al