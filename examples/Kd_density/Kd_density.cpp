#include <iostream>
#include <fstream>
#include <string>

#include <tinyopt/tinyopt.h>

#include <arblang/optimizer/optimizer.hpp>
#include <arblang/optimizer/inline_func.hpp>
#include <arblang/parser/parser.hpp>
#include <arblang/parser/normalizer.hpp>
#include <arblang/pre_printer/printable_mechanism.hpp>
#include <arblang/printer/print_header.hpp>
#include <arblang/printer/print_mechanism.hpp>
#include <arblang/resolver/canonicalize.hpp>
#include <arblang/resolver/resolve.hpp>
#include <arblang/resolver/single_assign.hpp>
#include <arblang/solver/solve.hpp>

const char* usage_str =
        "\n"
        "-o|--output            [Prefix for output file names]\n"
        "-N|--namespace         [Namespace for generated code]\n"
        "<filename>             [File to be compiled]\n";

int main(int argc, char **argv) {
    using namespace al;
    using namespace al::resolved_ir;
    using namespace to;

    std::string opt_namespace, opt_input, opt_output;
    try {
        std::vector<std::string> targets;

        auto help = [argv0 = argv[0]] {
            to::usage(argv0, usage_str);
        };

        to::option options[] = {
                { opt_input,  to::mandatory},
                { opt_output, "-o", "--output" },
                { opt_namespace, "-N", "--namespace" },
        };

        if (!to::run(options, argc, argv+1)) return 0;
    }
    catch (to::option_error& e) {
        to::usage_error(argv[0], usage_str, e.what());
        return 1;
    }

    // Read mechanism file
    std::string mech;
    try {
        std::ifstream fi;
        fi.exceptions(std::ios::failbit);
        fi.open(opt_input);
        mech.assign(std::istreambuf_iterator<char>(fi), std::istreambuf_iterator<char>());
    }
    catch (const std::exception&) {
        throw std::runtime_error("Failure opening" + opt_input);
    }

    auto p = parser(mech);

    auto m_parsed = p.parse_mechanism();
    auto m_normal = normalize(m_parsed);
    auto m_resolved = resolve(m_normal);
    auto m_canon = canonicalize(m_resolved);
    auto m_ssa = single_assign(m_canon);

    auto opt_0 = optimizer(m_ssa);
    auto m_opt = opt_0.optimize();

    auto m_inlined = inline_func(m_opt);

    auto opt_1 = optimizer(m_inlined);
    auto m_fin = opt_1.optimize();

    m_fin = solve(m_fin);
    auto m_printable = printable_mechanism(m_fin);

    std::ofstream fo_hpp, fo_cpp;
    fo_hpp.open(opt_output+".hpp");
    fo_hpp << print_header(m_printable, "namespace").str();
    fo_hpp.close();

    fo_cpp.open(opt_output+"_cpu.cpp");
    fo_cpp << print_mechanism(m_printable, "namespace").str();
    fo_cpp.close();
}