#include <iostream>

#include <arblang/lexer.hpp>

int main() {
    using namespace al;
    std::string module =
        "module foo {\n"
        "\n"
        "# \"voltage\" here specifies the quantity type of erev.\n"
        "\n"
        "parameter voltage erev = -23 mV;\n"
        "\n"
        "# The \"parameter\" keyword introduces a new parameter. The rhs should always be specified\n"
        "# as a default parameter value, but should allow expressions in terms of previously\n"
        "# declared parameter values too?\n"
        "\n"
        "parameter time tau = 2.0 ms;\n"
        "\n"
        "# There is nothing special about the name \"state\", it is just the name given to\n"
        "# the record type.\n"
        "\n"
        "record state {\n"
        "    conductance g;\n"
        "}\n"
        "\n"
        "# \"def\" introduces a function definition.\n"
        "\n"
        "def initial() -> state {\n"
        "    # The name of a record type also acts as its constructor.\n"
        "    # Every record field needs to be set, or else be taken from an existing value, or\n"
        "    # else have a default value given in the record definition.\n"
        "\n"
        "    state { g = 0; };\n"
        "}\n"
        "\n"
        "# \"state'\" is a struct that is automatically defined given a struct called \"state\".\n"
        "\n"
        "def evolution(state s, voltage v) -> state' {\n"
        "    state' {\n"
        "        # Record field access is by dot.\n"
        "        g' = -s.g / tau;\n"
        "    };\n"
        "}\n"
        "\n"
        "# But there's nothing magic about \"state'\"; we could instead write:\n"
        "\n"
        "record state_deriv {\n"
        "    conductance/time g' = 0;\n"
        "}\n"
        "\n"
        "def evolution2(state s, voltage v) -> state_deriv {\n"
        "    state_deriv {\n"
        "        g' = -s.g / tau;\n"
        "    };\n"
        "}\n"
        "\n"
        "# And maybe the record syntax is too clunky, so we can import field names into scope.\n"
        "# \"let\" and \"with\" statements introduce a new scope for the following expression, and\n"
        "# functions ultimately provide a single expression.\n"
        "\n"
        "def evolution3(state s, voltage v) -> state' {\n"
        "    with s; # equivalent to: \"let g = s.g\"\n"
        "    state' {\n"
        "        g' = g / tau;\n"
        "    };\n"
        "}\n"
        "\n"
        "# All of \"evolution\", \"evolution2\", and \"evolution3\" have the same functional type\n"
        "# and the same semantics, and should be able to be used interchangeably.\n"
        "\n"
        "# This is a synapse model, so we'll also need a function that describes what happens\n"
        "# on the receipt of a post-synaptic event.\n"
        "\n"
        "def on_event(state s, real weight) {\n"
        "    state {\n"
        "        g = s.g + weight * 1 uS  # explicit scaling of weight required for unit correctness.\n"
        "    };\n"
        "}\n"
        "\n"
        "} # end of module definition";

    lexer lex(module);
    while (true) {
        auto t = lex.parse();
        if (t.type == tok::error) {
            std::cout << "ERROR: " << lex.error_message() << std::endl;
            break;
        }
        std::cout << t << std::endl;
        if (t.type == tok::eof) break;
    }
    return 0;
}