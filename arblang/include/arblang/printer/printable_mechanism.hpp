#pragma once

#include "arblang/resolver/resolved_expressions.hpp"

namespace al {
namespace resolved_ir {

struct printable_mechanism {
    printable_mechanism() {};

    std::string name;
    mechanism_kind kind;
    std::vector<r_expr> constant_parameters;
    std::vector<r_expr> assigned_parameters;
    std::vector<r_expr> initializations;
    std::vector<r_expr> effects;
    std::vector<r_expr> evolutions;

    std::string pp_prefix = "_pp_";

    // Used to create assign storage for parameters and state vars
    // and create named pointers to the storage of parameters, state
    // vars, bindables and affectables.
    struct mechanism_fields {
        std::vector<std::string> param_sources;
        std::vector<std::string> state_sources;
        std::vector<std::tuple<bindable, std::optional<std::string>, std::string>> bind_sources;
        std::vector<std::tuple<affectable, std::optional<std::string>, std::string>> effect_sources;
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
};

} // namespace resolved_ir
} // namespace al