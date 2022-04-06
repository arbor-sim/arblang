#include <sstream>

#include <fmt/compile.h>
#include <fmt/core.h>

#include <arblang/printer/print_mechanism.hpp>
#include <arblang/printer/print_expressions.hpp>
#include <arblang/util/unique_name.hpp>

namespace std {
template <>
struct hash<al::resolved_ir::printable_mechanism::storage_info> {
    std::size_t operator()(const al::resolved_ir::printable_mechanism::storage_info& k) const {
        return std::hash<std::string>()(k.pointer_name);
    }
};
}

namespace al {
namespace resolved_ir {
bool operator==(const printable_mechanism::storage_info& lhs, const printable_mechanism::storage_info& rhs) {
    return lhs.pointer_name == rhs.pointer_name;
}

std::stringstream print_mechanism(const printable_mechanism& mech, const std::string& cpp_namespace) {
    std::stringstream out;

    // Define names for pointers to simulator defined indices and parameters
    static constexpr const char* mech_width        = "_pp_sim_width";
    static constexpr const char* mech_node_index   = "_pp_sim_node_index";
    static constexpr const char* mech_node_weight  = "_pp_sim_weight";
    static constexpr const char* mech_id           = "_pp_sim_mechanism_id";
    static constexpr const char* mech_ion_idx_pref = "_pp_sim_index_ion_";
    static constexpr const char* node_idx_var      = "_nidx";
    static constexpr const char* ion_idx_var_pref  = "_nidx_";

    // Print includes
    out << "#include <algorithm>\n"
           "#include <cmath>\n"
           "#include <cstddef>\n"
           "#include <memory>\n"
           "#include <arbor/mechanism_abi.h>\n"
           "#include <arbor/math.hpp>\n\n";

    // Open namespaces
    out << fmt::format("namespace arb {{\n");
    out << fmt::format("namespace {} {{\n", cpp_namespace);
    out << fmt::format("namespace kernel_{} {{\n\n", mech.mech_name);

    // Print aliases && constexpr
    out << "using ::arb::math::exprelr;\n"
           "using ::arb::math::safeinv;\n"
           "using ::std::abs;\n"
           "using ::std::cos;\n"
           "using ::std::exp;\n"
           "using ::std::log;\n"
           "using ::std::max;\n"
           "using ::std::min;\n"
           "using ::std::pow;\n"
           "using ::std::sin;\n"
           "\n";

    out << "static constexpr unsigned simd_width_ = 1;\n"  // TODO change when we implement vectorization
           "static constexpr unsigned min_align_ = std::max(alignof(arb_value_type), alignof(arb_index_type));\n\n";

    // Define PPACK_IFACE_BLOCK

    // Print the pointers that are always expected to be available
    out << "#define PPACK_IFACE_BLOCK \\\n";
    out << fmt::format("[[maybe_unused]] auto  {} = pp->width;\\\n", mech_width);
    out << fmt::format("[[maybe_unused]] auto* {} = pp->node_index;\\\n", mech_node_index);
    out << fmt::format("[[maybe_unused]] auto* {} = pp->weight;\\\n", mech_node_weight);
    out << fmt::format("[[maybe_unused]] auto& {} = pp->mechanism_id;\\\n", mech_id);

    unsigned idx = 0;
    for (const auto& item: mech.ionic_fields) {
        out << fmt::format("[[maybe_unused]] auto& {}{} = pp->ion_states[{}].index;\\\n",
                           mech_ion_idx_pref, item.ion, idx++);
    }

    // Print the pointers to the bindables
    for (const auto& [name, bind, ion]: mech.field_pack.bind_sources) {
        if (!mech.pointer_map.count(name)) {
            throw std::runtime_error(fmt::format("Internal compiler error: 0 sources found for {}.",
                                                 name));
        }
        auto pointer_name = mech.pointer_map.at(name).pointer_name;
        // If there's no associated ion;
        if (!ion) {
            switch (bind) {
                case bindable::membrane_potential:
                    out << fmt::format("[[maybe_unused]] auto* {} = pp->vec_v;\\\n", pointer_name);
                    break;
                case bindable::temperature:
                    out << fmt::format("[[maybe_unused]] auto* {} = pp->temperature_degC;\\\n", pointer_name);
                    break;
                case bindable::dt:
                    out << fmt::format("[[maybe_unused]] auto* {} = pp->vec_dt;\\\n", pointer_name);
                    break;
                default:
                     throw std::runtime_error(fmt::format("Internal compiler error: bindable {} expects an ion species",
                                                          to_string(bind)));
            }
        }
        else {
            auto ion_name = ion.value();
            unsigned ion_idx = 0;
            for (const auto& item: mech.ionic_fields) {
                if (item.ion == ion_name) break;
                ion_idx++;
            }

            switch (bind) {
                case bindable::current_density:
                    out << fmt::format("[[maybe_unused]] auto* {} = pp->ion_states[{}].current_density;\\\n",
                                       pointer_name, ion_idx);
                    break;
                case bindable::charge:
                    out << fmt::format("[[maybe_unused]] auto* {} = pp->ion_states[{}].internal_concentration;\\\n",
                                       pointer_name, ion_idx);
                    break;
                case bindable::internal_concentration:
                    out << fmt::format("[[maybe_unused]] auto* {} = pp->ion_states[{}].external_concentration;\\\n",
                                       pointer_name, ion_idx);
                    break;
                case bindable::external_concentration:
                    out << fmt::format("[[maybe_unused]] auto* {} = pp->ion_states[{}].ionic_charge;\\\n",
                                       pointer_name, ion_idx);
                    break;
                default:
                    throw std::runtime_error(fmt::format("Internal compiler error: bindable {} doesn't expect an "
                                                         "ion species", to_string(bind)));
            }
        }
    }

    // Print the pointers to the affectables
    for (const auto& [name, effect, ion]: mech.field_pack.effect_sources) {
        if (!mech.pointer_map.count(name)) {
            throw std::runtime_error(fmt::format("Internal compiler error: 0 sources found for {}.",
                                                 name));
        }
        auto pointer_name = mech.pointer_map.at(name).pointer_name;

        // If there's no associated ion;
        if (!ion) {
            switch (effect) {
                case affectable::current_density:
                case affectable::current:
                    out << fmt::format("[[maybe_unused]] auto* {} = pp->vec_i;\\\n", pointer_name);
                    break;
                case affectable::conductance:
                case affectable::conductivity:
                    out << fmt::format("[[maybe_unused]] auto* {} = pp->vec_g;\\\n", pointer_name);
                    break;
                default: break; // TODO add other affectables when implemented
            }
        }
        else {
            auto ion_name = ion.value();
            unsigned ion_idx = 0;
            for (const auto& item: mech.ionic_fields) {
                if (item.ion == ion_name) break;
                ion_idx++;
            }
            switch (effect) {
                case affectable::current_density:
                case affectable::current:
                    out << fmt::format("[[maybe_unused]] auto* {} = pp->ion_states[{}].current_density;\\\n",
                                       pointer_name, ion_idx);
                    break;
                default: break; // TODO add other affectables when implemented
            }
        }
    }
    // Print the pointers to the parameters
    idx = 0;
    for (const auto& [name, val, unit]: mech.field_pack.param_sources) {
        if (!mech.pointer_map.count(name)) {
            throw std::runtime_error(fmt::format("Internal compiler error: 0 sources found for {}.",
                                                 name));
        }
        auto pointer_name = mech.pointer_map.at(name).pointer_name;
        out << fmt::format("[[maybe_unused]] auto* {} = pp->parameters[{}];\\\n", pointer_name, idx);
        idx++;
    }

    // Print the pointers to the states
    idx = 0;
    for (const auto& name: mech.field_pack.state_sources) {
        if (!mech.pointer_map.count(name)) {
            throw std::runtime_error(fmt::format("Internal compiler error: 0 sources found for {}.",
                                                 name));
        }
        auto pointer_name = mech.pointer_map.at(name).pointer_name;
        out << fmt::format("[[maybe_unused]] auto* {} = pp->state_vars[{}];\\\n", pointer_name, idx);
        idx++;
    }
    out << "\n";

    // printer helpers
    struct index_info {
        bool external_access;
        std::unordered_set<std::string> ions_accessed;
    };
    auto check_access = [](const auto& map) -> index_info {
        bool external_access = false;
        std::unordered_set<std::string> ions_accessed;
        for (const auto& [var, ptr]: map) {
            switch (ptr.pointer_kind) {
                case printable_mechanism::storage_class::ionic:
                    ions_accessed.insert(ptr.ion.value());
                case printable_mechanism::storage_class::external:
                    external_access = true;
                default: break;
            }
        }
        return {external_access, ions_accessed};
    };
    auto print_read = [&](const auto& map, const std::string& indent) {
        for (const auto& [var, ptr]: map) {
            switch (ptr.pointer_kind) {
                case printable_mechanism::storage_class::ionic:
                    if (ptr.scale) {
                        out << fmt::format("{}auto {} = {}[{}{}]*{};\n", indent, var, ptr.pointer_name, ion_idx_var_pref, ptr.ion.value(), ptr.scale.value());
                    } else {
                        out << fmt::format("{}auto {} = {}[{}{}];\n", indent, var, ptr.pointer_name, ion_idx_var_pref, ptr.ion.value());
                    }
                    break;
                case printable_mechanism::storage_class::external:
                    if (ptr.scale) {
                        out << fmt::format("{}auto {} = {}[{}]*{};\n", indent, var, ptr.pointer_name, node_idx_var, ptr.scale.value());
                    } else {
                        out << fmt::format("{}auto {} = {}[{}];\n", indent, var, ptr.pointer_name, node_idx_var);
                    }
                    break;
                case printable_mechanism::storage_class::internal:
                    if (ptr.scale) {
                        out << fmt::format("{}auto {} = {}[i_]*{};\n", indent, var, ptr.pointer_name, ptr.scale.value());
                    } else {
                        out << fmt::format("{}auto {} = {}[i_];\n", indent, var, ptr.pointer_name);
                    }
                    break;
                default: break;
            }
        }
    };
    auto print_write = [&](const auto& map, const std::string& indent) {
        // If an external or ionic storage class is written to multiple times,
        // write the sum of the variables only once.
        std::unordered_map<printable_mechanism::storage_info, std::vector<std::string>> reduced_map;
        for (const auto& [var, ptr]: map) {
            reduced_map[ptr].push_back(var);
        }
        std::unordered_set<std::string> reserved_names;
        for (const auto& [ptr, vars]: reduced_map) {
            std::string var_name = vars.front();
            if (vars.size() > 1) {
                // Sum up the contributions
                var_name = unique_local_name(reserved_names, "sum");
                std::string var_val;
                bool first = true;
                for (const auto& v: vars) {
                    if (!first) var_val += " + ";
                    var_val += v;
                    first = false;
                }
                out << fmt::format("{}auto {} = {};\n", indent, var_name, var_val);
            }
            switch (ptr.pointer_kind) {
                case printable_mechanism::storage_class::ionic:
                    if (ptr.scale) {
                        out << fmt::format("{6}{0}[{1}{2}] = fma({5}*{3}[i_], {4}, {0}[{1}{2}]);\n",
                                           ptr.pointer_name, ion_idx_var_pref, ptr.ion.value(), mech_node_weight, var_name, ptr.scale.value(), indent);
                    } else {
                        out << fmt::format("{5}{0}[{1}{2}] = fma({3}[i_], {4}, {0}[{1}{2}]);\n",
                                           ptr.pointer_name, ion_idx_var_pref, ptr.ion.value(), mech_node_weight, var_name, indent);
                    }
                    break;
                case printable_mechanism::storage_class::external:
                    if (ptr.scale) {
                        out << fmt::format("{5}{0}[{1}] = fma({4}*{2}[i_], {3}, {0}[{1}]);\n",
                                           ptr.pointer_name, node_idx_var, mech_node_weight, var_name, ptr.scale.value(), indent);
                    } else {
                        out << fmt::format("{4}{0}[{1}] = fma({2}[i_], {3}, {0}[{1}]);\n",
                                           ptr.pointer_name, node_idx_var, mech_node_weight, var_name, indent);
                    }
                    break;
                case printable_mechanism::storage_class::internal:
                    if (ptr.scale) {
                        out << fmt::format("{3}{0}[i_] = {2}*{1};\n", ptr.pointer_name, var_name, ptr.scale.value(), indent);
                    } else {
                        out << fmt::format("{2}{0}[i_] = {1};\n", ptr.pointer_name, var_name, indent);
                    }
                    break;
                default: break;
            }
        }
    };

    // print init
    {
        out << fmt::format("static void init(arb_mechanism_ppack* pp) {{\n");
        if (!(mech.procedure_pack.assigned_parameters.empty() && mech.procedure_pack.initializations.empty())) {
            out << fmt::format("    PPACK_IFACE_BLOCK;\n");
            out << fmt::format("    for (arb_size_type i_ = 0; i_ < {}; ++i_) {{\n", mech_width);

            auto read_access = check_access(mech.init_read_map);
            auto write_access = check_access(mech.init_write_map);

            // print indices
            if (read_access.external_access || write_access.external_access) {
                out << fmt::format("       auto {} = {}[i_];\n", node_idx_var, mech_node_index);
            }
            read_access.ions_accessed.merge(write_access.ions_accessed);
            for (const auto &ion: read_access.ions_accessed) {
                out << fmt::format("       auto {0}{2} = {1}{2}[i_];\n", ion_idx_var_pref, mech_ion_idx_pref, ion);
            }
            // print reads
            out << "       // Perform memory reads\n";
            print_read(mech.init_read_map, "       ");

            // print expressions
            out << "       // Perform calculations\n";
            for (const auto &p: mech.procedure_pack.assigned_parameters) {
                print_expression(p, out, "       ");
            }
            for (const auto &p: mech.procedure_pack.initializations) {
                print_expression(p, out, "       ");
            }

            // print writes
            out << "       // Perform memory writes\n";
            print_write(mech.init_write_map, "       ");

            out << fmt::format("    }}\n");
        }
        out << fmt::format("}}\n");
    }
    // print state
    {

        out << fmt::format("static void advance_state(arb_mechanism_ppack* pp) {{\n");
        if (!mech.procedure_pack.evolutions.empty()) {
            out << fmt::format("    PPACK_IFACE_BLOCK;\n");
            out << fmt::format("    for (arb_size_type i_ = 0; i_ < {}; ++i_) {{\n", mech_width);

            auto read_access = check_access(mech.evolve_read_map);
            auto write_access = check_access(mech.evolve_write_map);

            // print indices
            if (read_access.external_access || write_access.external_access) {
                out << fmt::format("       auto {} = {}[i_];\n", node_idx_var, mech_node_index);
            }
            read_access.ions_accessed.merge(write_access.ions_accessed);
            for (const auto &ion: read_access.ions_accessed) {
                out << fmt::format("       auto {0}{2} = {1}{2}[i_];\n", ion_idx_var_pref, mech_ion_idx_pref, ion);
            }
            // print reads
            out << "       // Perform memory reads\n";
            print_read(mech.evolve_read_map, "       ");

            // print expressions
            for (const auto &p: mech.procedure_pack.evolutions) {
                out << "       // Perform calculations\n";
                print_expression(p, out, "       ");
            }

            // print writes
            out << "       // Perform memory writes\n";
            print_write(mech.evolve_write_map, "       ");

            out << fmt::format("    }}\n");
        }
        out << fmt::format("}}\n");
    }
    // print current
    {
        out << fmt::format("static void compute_currents(arb_mechanism_ppack* pp) {{\n");
        if (!mech.procedure_pack.effects.empty()) {
            out << fmt::format("    PPACK_IFACE_BLOCK;\n");
            out << fmt::format("    for (arb_size_type i_ = 0; i_ < {}; ++i_) {{\n", mech_width);

            auto read_access = check_access(mech.effect_read_map);
            auto write_access = check_access(mech.effect_write_map);

            if (read_access.external_access || write_access.external_access) {
                out << fmt::format("       auto {} = {}[i_];\n", node_idx_var, mech_node_index);
            }
            read_access.ions_accessed.merge(write_access.ions_accessed);
            for (const auto &ion: read_access.ions_accessed) {
                out << fmt::format("       auto {0}{2} = {1}{2}[i_];\n", ion_idx_var_pref, mech_ion_idx_pref, ion);
            }
            // print reads
            out << "       // Perform memory reads\n";
            print_read(mech.effect_read_map, "       ");

            // print expressions
            for (const auto &p: mech.procedure_pack.effects) {
                out << "       // Perform calculations\n";
                print_expression(p, out, "       ");
            }

            // print writes
            out << "       // Perform memory writes\n";
            print_write(mech.effect_write_map, "       ");

            out << fmt::format("    }}\n");
        }
        out << fmt::format("}}\n");
    }
    // print apply_events
    {
        auto read_access = check_access(mech.event_read_map);
        auto write_access = check_access(mech.event_write_map);
        out << fmt::format(FMT_COMPILE("static void apply_events(arb_mechanism_ppack* pp, arb_deliverable_event_stream* stream_ptr) {{\n"));

        if (!mech.procedure_pack.on_events.empty()) {
            out << fmt::format(FMT_COMPILE("    PPACK_IFACE_BLOCK;\n"
                                           "    auto ncell = stream_ptr->n_streams;\n"
                                           "    for (arb_size_type c = 0; c<ncell; ++c) {{\n"
                                           "        auto begin  = stream_ptr->events + stream_ptr->begin[c];\n"
                                           "        auto end    = stream_ptr->events + stream_ptr->end[c];\n"
                                           "        for (auto p = begin; p<end; ++p) {{\n"
                                           "            auto i_     = p->mech_index;\n"));

            // find the and read stream members
            for (const auto&[var, ptr]: mech.event_read_map) {
                if (ptr.pointer_kind == printable_mechanism::storage_class::stream_member) {
                    out << fmt::format("            auto {}     = p->{};\n", var, ptr.pointer_name);
                }
            }
            out << fmt::format(FMT_COMPILE("            if (p->mech_id=={0}) {{\n"), mech_id);

            // print reads
            out << "                // Perform memory reads\n";
            print_read(mech.event_read_map, "                ");

            // print expressions
            for (const auto &p: mech.procedure_pack.on_events) {
                out << "                // Perform calculations\n";
                print_expression(p, out, "                ");
            }

            // print writes
            out << "                // Perform memory writes\n";
            print_write(mech.event_write_map, "                ");
            out << "            }\n"
                   "        }\n"
                   "    }\n";
        }
        out << "}\n";
    }
    // print write_ions, post_events (empty)
    {
        out << fmt::format("static void write_ions(arb_mechanism_ppack*) {{}}\n");
        out << fmt::format("static void post_event(arb_mechanism_ppack*) {{}}\n");
    }
    // undef PPACK_IFACE_BLOCK

    // Close namespaces
    out << "#undef PPACK_IFACE_BLOCK\n";
    out << fmt::format("}} // namespace arb\n");
    out << fmt::format("}} // namespace {} {{\n", cpp_namespace);
    out << fmt::format("}} // namespace kernel_{} {{\n", mech.mech_name);

    // define extern "C"
    std::string full_namespace = "arb::" + cpp_namespace + "::kernel_" + mech.mech_name;
    out << fmt::format("extern \"C\" {{\n");
    out << fmt::format("  arb_mechanism_interface* make_arb_{}_catalogue_{}_interface_multicore() {{\n", cpp_namespace, mech.mech_name);
    out << fmt::format("    static arb_mechanism_interface result;\n");
    out << fmt::format("    result.partition_width = {}::simd_width_;\n", full_namespace);
    out << fmt::format("    result.backend = arb_backend_kind_cpu;\n");
    out << fmt::format("    result.alignment = {}::min_align_;\n", full_namespace);
    out << fmt::format("    result.init_mechanism = {}::init;\n", full_namespace);
    out << fmt::format("    result.compute_currents = {}::compute_currents;\n", full_namespace);
    out << fmt::format("    result.apply_events = {}::apply_events;\n", full_namespace);
    out << fmt::format("    result.advance_state = {}::advance_state;\n", full_namespace);
    out << fmt::format("    result.write_ions = {}::write_ions;\n", full_namespace);
    out << fmt::format("    result.post_event = {}::post_event;\n", full_namespace);
    out << fmt::format("    return &result;\n");
    out << fmt::format("  }}}}");
    return out;
}

} // namespace resolved_ir
} // namespace al