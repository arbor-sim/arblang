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
        throw std::runtime_error("Failure opening " + opt_input);
    }

    // Parse the mechanism.
    // Produces `parsed_expressions`.
    auto p = parser(mech);
    auto m_parsed = p.parse_mechanism();
    // Normalize any units used: 1 mV -> 0.001 V.
    // Units can only appear after integer or float expressions.
    // Produces `parsed_expressions`.
    auto m_normal = normalize(m_parsed);

    // Resolve the mechanism.
    // Produces `resolved_expressions`, the main IR.
    // Performs type checking and name resolution.
    auto m_resolved = resolve(m_normal);

    // Canonicalize the mechanism.
    // Required before we can perform start optimization.
    // Ensures that the rhs of an assignment `=` is a single, un-nested, expression.
    // Produces `resolved_expressions`.
    auto m_canon = canonicalize(m_resolved);

    // Make sure that all variables are assigned only once.
    // There is no restriction on the user code to bind the same variable twice
    //   e.g. let a = 3; let a = 4; a; is valid and is equal to 4.
    //   It is also needed for `with_expressions`.
    //   If we decide to remove `with_expressions` and disallow double
    //   binding on variables this can be removed.
    // Produces `resolved_expressions`.
    auto m_ssa = single_assign(m_canon);

    // Optimize the mechanism.
    // Performs CSE, constant folding, copy propagation and dead-code elimination
    //   in a loop until no more changes can be made.
    // Produces `resolved_expressions`.
    auto opt_0 = optimizer(m_ssa);
    auto m_opt = opt_0.optimize();

    // Inline functions.
    // Produces `resolved_expressions`.
    auto m_inlined = inline_func(m_opt);

    // Reoptimize after inlining.
    // Produces `resolved_expressions`.
    auto opt_1 = optimizer(m_inlined);
    auto m_fin = opt_1.optimize();

    // Solve the mechanism.
    // Entails solving any ODEs and finding the conductance.
    // Only simple diagonal systems of ODEs are supported.
    // Requires a prefix for the current and conductance
    //   variables that will also be used during the printing
    //   stage.
    // Produces `resolved_expressions`.
    std::string i_name = "i";
    std::string g_name = "g";
    m_fin = solve(m_fin, i_name, g_name);

    // Prepare the mechanism for printing.
    // Gathers information about which variables are read/written
    //   in each kernel and their kinds.
    auto m_printable = printable_mechanism(m_fin, i_name, g_name);

    // Print the mechanism.
    // Generate C++ code written against arbor's mechanism ABI.
    std::ofstream fo_hpp, fo_cpp;
    fo_hpp.open(opt_output+".hpp");
    fo_hpp << print_header(m_printable, opt_namespace).str();
    fo_hpp.close();

    fo_cpp.open(opt_output+"_cpu.cpp");
    fo_cpp << print_mechanism(m_printable, opt_namespace).str();
    fo_cpp.close();
}