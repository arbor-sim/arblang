#pragma once

#include <unordered_set>
#include <unordered_map>

#include <arblang/resolver/resolved_expressions.hpp>

namespace al {
namespace resolved_ir {

using record_field_map = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

struct printable_mechanism {
    printable_mechanism(const resolved_mechanism& m, std::string i_name, std::string g_name);

    std::string mech_name;
    mechanism_kind mech_kind;

    struct mechanism_procedures {
        std::vector<r_expr> assigned_parameters;
        std::vector<r_expr> initializations;
        std::vector<r_expr> on_events;
        std::vector<r_expr> effects;
        std::vector<r_expr> evolutions;
    } procedure_pack;

    // Used to assign storage for parameters and state vars,
    // and to create named pointers to the storage of parameters,
    // state vars, bindables and affectables.
    struct mechanism_fields {
        std::vector<std::tuple<std::string, double, std::string>> param_sources; // param name to val and unit
        std::vector<std::string> state_sources;
        std::vector<std::tuple<std::string, bindable, std::optional<std::string>>> bind_sources;
        std::vector<std::tuple<std::string, affectable, std::optional<std::string>>> effect_sources;
    } field_pack;

    struct ion_info {
        std::string ion;
        bool read_valence;
        bool write_int_concentration;
        bool write_ext_concentration;
    };
    // This needs to be a vector, we rely on it for indexing
    std::vector<ion_info> ionic_fields;

    enum class storage_class {
        internal,      // param, state
        external,      // indexed by node_index
        ionic,         // indexed by ionic node_index
        stream_member, // A member of an event stream (weight)
    };
    struct storage_info {
        std::string pointer_name;
        storage_class pointer_kind;
        std::optional<std::string> ion;
        std::optional<double> scale;
    };

    // Map from variable name to source/destination pointer storage info.
    // write_map is a multimap because certain destinations can be written
    // multiple times. These get accumulated durign the printing stage.
    using write_map = std::unordered_multimap<std::string, storage_info>;
    using read_map = std::unordered_map<std::string, storage_info>;

    // Map from all parameters/states/bindables/affectables to prefixed pointer names of the data storage.
    // e.g. `state s: real; parameter p: voltage = 1 [V]; bindable b = membrane_voltage; effect current_density = ...`
    // source_pointer_map = {
    //   {s: {_pp_s, internal}},
    //   {p: {_pp_p, internal}},
    //   {b: {_pp_b, external}},
    //   {_effect_i: {_pp__effect_i, external}},
    //   {_effect_g: {_pp__effect_g, external}},
    // }
    read_map pointer_map;

    // Per stage map from writable/readable variables to prefixed pointer names of the destinations/sources.
    read_map  init_read_map;
    write_map init_write_map;

    read_map  event_read_map;
    write_map event_write_map;

    read_map  effect_read_map;
    write_map effect_write_map;

    read_map  evolve_read_map;
    write_map evolve_write_map;

private:
    std::string pp_prefix_ = "_pp_";
    std::string effect_rec_name_ = "effect";
    std::string current_field_name_;
    std::string conductance_field_name_;

    std::string prefix(const std::string& n) { return pp_prefix_ + n; }

    std::string mangle(const std::string& pre, const std::string& field) {
        return "_"  + pre + "_" + field;
    };

    record_field_map gen_record_field_map(const std::vector<std::pair<std::string, r_type>>&);
    resolved_mechanism simplify_mech(const resolved_mechanism& mech, const record_field_map& map);

    void fill_write_maps(const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>&,
                         const write_map&);
    void fill_read_maps();
};

} // namespace resolved_ir
} // namespace al