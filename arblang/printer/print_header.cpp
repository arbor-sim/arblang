#include <regex>
#include <sstream>

#include <fmt/compile.h>
#include <fmt/core.h>

#include <arblang/printer/print_header.hpp>

namespace al {
namespace resolved_ir {

inline const char* arb_header_prefix() {
    static const char* prefix = "arbor/";
    return prefix;
}

inline const char* arb_mechanism_kind(const mechanism_kind& m) {
    switch (m) {
        case mechanism_kind::density:       return "arb_mechanism_kind_density";
        case mechanism_kind::point:         return "arb_mechanism_kind_point";
        case mechanism_kind::concentration: return "arb_mechanism_kind_density";
        case mechanism_kind::junction:      return "arb_mechanism_kind_gap_junction";
    }
    return "";
}

std::stringstream print_header(
    const printable_mechanism& mech,
    const std::string& cpp_namespace,
    bool cpu, bool gpu)
{
    std::stringstream out;
    std::string fingerprint = "<placeholder>";

    const std::string min = "1e-9";
    const std::string max    = "1e9";
    out << fmt::format("#pragma once\n\n"
                       "#include <cmath>\n"
                       "#include <{}mechanism_abi.h>\n\n",
                       arb_header_prefix());

    out << fmt::format("extern \"C\" {{\n"
                       "  arb_mechanism_type make_{0}_{1}() {{\n"
                       "    // Tables\n",
                       std::regex_replace(cpp_namespace, std::regex{"::"}, "_"),
                       mech.name);

    // print states:
    out << "    static arb_field_info state_vars[] = {\n";
    for (const auto& p: mech.field_pack.state_sources) {
        out << fmt::format("        {{\"{}\", \"{}\", {}, {}, {}}}, \n", p, "", "NAN", min, max);
    }
    out << "    };\n";
    out << "    static arb_size_type n_state_vars = " << mech.field_pack.state_sources.size() << ";\n";

    // print parameters:
    out << "    static arb_field_info parameters[] = {\n";
    for (const auto& [p, val, unit]: mech.field_pack.param_sources) {
        out << fmt::format("        {{\"{}\", \"{}\", {}, {}, {}}}, \n", p, unit, val, min, max);
    }
    out << "    };\n";
    out << "    static arb_size_type n_parameters = " << mech.field_pack.param_sources.size() << ";\n";

    // print ions:
    out << "    static arb_ion_info ions[] = {\n";
    for (const auto& ion: mech.ionic_fields) {
        out << fmt::format("        {{\"{}\", {}, {}, false, false, {}, false, 0}}, \n",
                           ion.ion, ion.write_int_concentration, ion.write_int_concentration, ion.read_valence);
    }
    out << "    };\n";
    out << "    static arb_size_type n_ions = " << mech.ionic_fields.size() << ";\n";

    out << fmt::format(FMT_COMPILE("\n"
                                   "    arb_mechanism_type result;\n"
                                   "    result.abi_version=ARB_MECH_ABI_VERSION;\n"
                                   "    result.fingerprint=\"{1}\";\n"
                                   "    result.name=\"{0}\";\n"
                                   "    result.kind={2};\n"
                                   "    result.is_linear={3};\n"
                                   "    result.has_post_events={4};\n"
                                   "    result.globals=globals;\n"
                                   "    result.n_globals=n_globals;\n"
                                   "    result.ions=ions;\n"
                                   "    result.n_ions=n_ions;\n"
                                   "    result.state_vars=state_vars;\n"
                                   "    result.n_state_vars=n_state_vars;\n"
                                   "    result.parameters=parameters;\n"
                                   "    result.n_parameters=n_parameters;\n"
                                   "    return result;\n"
                                   "  }}\n"
                                   "\n"),
                       mech.name,
                       fingerprint,
                       arb_mechanism_kind(mech.kind),
                       false, // TODO: actually check linearity
                       false) // TODO: actually check post_events
        << fmt::format("  arb_mechanism_interface* make_{0}_{1}_interface_multicore(){2}\n"
                       "  arb_mechanism_interface* make_{0}_{1}_interface_gpu(){3}\n"
                       "}}\n",
                       std::regex_replace(cpp_namespace, std::regex{"::"}, "_"),
                       mech.name,
                       cpu ? ";" : " { return nullptr; }",
                       gpu ? ";" : " { return nullptr; }");
    return out;
}

} // namespace resolved_ir
} // namespace al