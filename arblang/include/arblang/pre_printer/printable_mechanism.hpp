#pragma once

#include <unordered_set>
#include <unordered_map>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

using state_field_map = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

struct printable_mechanism {
    printable_mechanism(const resolved_mechanism& m);

    std::string name;
    mechanism_kind kind;

    struct mechanism_procedures {
        std::vector<r_expr> constant_parameters;
        std::vector<r_expr> assigned_parameters;
        std::vector<r_expr> initializations;
        std::vector<r_expr> effects;
        std::vector<r_expr> evolutions;
    } procedure_pack;

    // Used to create assign storage for parameters and state vars
    // and create named pointers to the storage of parameters, state
    // vars, bindables and affectables.
    struct mechanism_fields {
        std::unordered_set<std::string> param_sources;
        std::unordered_set<std::string> state_sources;
        std::unordered_map<std::string, std::pair<bindable, std::optional<std::string>>> bind_sources;
        std::unordered_map<std::string, std::pair<affectable, std::optional<std::string>>> effect_sources;
    } field_pack;

    struct read_map {
        // Maps from source (pointer name) to variable name
        std::unordered_map<std::string, std::string> state_map;
        std::unordered_map<std::string, std::string> parameter_map;
        std::unordered_map<std::string, std::string> binding_map;
    };

    struct write_map {
        // Maps from target (pointer name) to variable name
        std::unordered_map<std::string, std::string> state_map;
        std::unordered_map<std::string, std::string> parameter_map;
        std::unordered_map<std::string, std::string> effect_map;
    };

    read_map  init_read_map;
    write_map init_write_map;

    read_map  effect_read_map;
    write_map effect_write_map;

    read_map  evolve_read_map;
    write_map evolve_write_map;

private:
    std::string pp_prefix_ = "_pp_";
    std::string prefix(const std::string& n) { return pp_prefix_ + n; }

    static state_field_map gen_state_field_map(const std::vector<r_expr>& state_declarations);

    void fill_write_maps(const std::unordered_map<std::string, std::string>&,
                         const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>&);
    void fill_read_maps(const std::unordered_map<std::string, std::string>&);
};

} // namespace resolved_ir
} // namespace al