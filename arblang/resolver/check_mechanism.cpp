#include <fmt/core.h>

#include <arblang/resolver/check_mechanism.hpp>

namespace al {
namespace resolved_ir {

// Check mechanism
void check(const resolved_mechanism& e) {
    for (auto& a: e.bindings) {
        auto b = std::get<resolved_bind>(*a);
        if ((e.kind != mechanism_kind::concentration) &&
            ((b.bind == bindable::molar_flux) || (b.bind == bindable::current_density)))
        {
            throw std::runtime_error(fmt::format("Unsupported bindable {} for mechanism kind {} at {}",
                                                 to_string(b.bind), to_string(e.kind), to_string(e.loc)));
        }
    }
    for (auto& a: e.effects) {
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
    for (auto& a: e.effects) {
        auto b = std::get<resolved_effect>(*a);
        if (b.effect != affectable::current_density && b.effect != affectable::current) {
            throw std::runtime_error(fmt::format("Unsupported effect {} at {}, still a work in progress.",
                                                 to_string(b.effect), to_string(e.loc)));
        }
    }
}

} // namespace resolved_ir
} // namespace al