#include <iomanip>
#include <limits>

#include <arblang/util/pretty_printer.hpp>

namespace al {
using namespace resolved_type_ir;
using namespace resolved_ir;

//r_expr
std::string pretty_print(const resolved_mechanism& e) {
    std::string str = e.name + " " + to_string(e.kind) + " {\n";
    for (const auto& p: e.parameters) {
        str += pretty_print(p) + "\n";
    }
    for (const auto& p: e.constants) {
        str += pretty_print(p) + "\n";
    }
    for (const auto& p: e.states) {
        str += pretty_print(p) + "\n";
    }
    for (const auto& p: e.bindings) {
        str += pretty_print(p) + "\n";
    }
    for (const auto& p: e.functions) {
        str += pretty_print(p) + "\n";
    }
    for (const auto& p: e.initializations) {
        str += pretty_print(p) + "\n";
    }
    for (const auto& p: e.on_events) {
        str += pretty_print(p) + "\n";
    }
    for (const auto& p: e.evolutions) {
        str += pretty_print(p) + "\n";
    }
    for (const auto& p: e.effects) {
        str += pretty_print(p) + "\n";
    }
    for (const auto& p: e.exports) {
        str += pretty_print(p) + "\n";
    }
    return str + "}";
}

std::string pretty_print(const resolved_parameter& e) {
    std::string str =  "parameter " + e.name + ":" + pretty_print(e.type) + " =\n" + pretty_print(e.value);
    if (str.back() != ';') str += ";";
    return str;
}

std::string pretty_print(const resolved_constant& e) {
    std::string str = "constant " + e.name + ":" + pretty_print(e.type) + " =\n" + pretty_print(e.value);
    if (str.back() != ';') str += ";";
    return str;
}

std::string pretty_print(const resolved_state& e) {
    return "state " + e.name + ":" + pretty_print(e.type) + ";";
}

std::string pretty_print(const resolved_record_alias& e) {
    return "record" + e.name + ":" + pretty_print(e.type) + ";";
}

std::string pretty_print(const resolved_function& e) {
    std::string str = "function " + e.name + "(";
    bool first = true;
    for (const auto& f: e.args) {
        if (!first) str += ", ";
        str += pretty_print(f) + ":" + pretty_print(type_of(f));
        first = false;
    }
    str += "):" + pretty_print(e.type) + "{\n";
    str += pretty_print(e.body) + "\n};";
    return str;
}

std::string pretty_print(const resolved_bind& e) {
    std::string str = "bind " + e.name + ":" + pretty_print(e.type) + " = " + to_string(e.bind);
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    return str + ";";
}

std::string pretty_print(const resolved_initial& e) {
    std::string str = "initial " + pretty_print(e.identifier) + ":" + pretty_print(e.type) +
                    " =\n" + pretty_print(e.value);
    if (str.back() != ';') str += ";";
    return str;
}

std::string pretty_print(const resolved_on_event& e) {
    std::string str = "on_event(" + pretty_print(e.argument) + ") " +
                      pretty_print(e.identifier) + ":" + pretty_print(e.type) +
                      " =\n" + pretty_print(e.value);
    if (str.back() != ';') str += ";";
    return str;
}

std::string pretty_print(const resolved_evolve& e) {
    std::string str = "evolve " + pretty_print(e.identifier) + ":" + pretty_print(e.type) +
                      " =\n" + pretty_print(e.value);
    if (str.back() != ';') str += ";";
    return str;
}

std::string pretty_print(const resolved_effect& e) {
    std::string str = "effect " + to_string(e.effect);
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += ":" + pretty_print(e.type) + " =\n" + pretty_print(e.value);
    if (str.back() != ';') str += ";";
    return str;
}

std::string pretty_print(const resolved_export& e) {
    return "export " + pretty_print(e.identifier) + ":" + pretty_print(e.type) + ";";
}

std::string pretty_print(const resolved_call& e) {
    std::string str = e.f_identifier + "(";
    bool first = true;
    for (const auto& f: e.call_args) {
        if (!first) str += ", ";
        str += pretty_print(f);
        first = false;
    }
    return str + ")";
}

std::string pretty_print(const resolved_object& e) {
    std::string str = "{";

    auto names = e.field_names();
    auto values = e.field_values();
    bool first = true;
    for (unsigned i = 0; i < names.size(); ++i) {
        if (!first) str += " ";
        str += names[i] + " = " + pretty_print(values[i]) + ";";
        first = false;
    }
    return str + "}";
}

std::string pretty_print(const resolved_let& e) {
    std::string str = "let " + e.id_name() + ":" + pretty_print(type_of(e.id_value())) + " = " +
                      pretty_print(e.id_value()) + ";\n" + pretty_print(e.body);
    if (str.back() != ';') str += ";";
    return str;
}

std::string pretty_print(const resolved_conditional& e) {
    return pretty_print(e.condition) + "? "+ pretty_print(e.value_true) + ": " + pretty_print(e.value_false);
}

std::string pretty_print(const resolved_float& e) {
    std::ostringstream os;
    os << std::setprecision(std::numeric_limits<double>::max_digits10) << e.value;
    return os.str() + ":" + pretty_print(e.type);
}

std::string pretty_print(const resolved_int& e) {
    return std::to_string(e.value) + ":" + pretty_print(e.type);
}

std::string pretty_print(const resolved_unary& e) {
    if (e.op == unary_op::lnot || e.op == unary_op::neg)
        return to_string(e.op) + pretty_print(e.arg);
    return to_string(e.op) + "(" + pretty_print(e.arg) + ")";
}

std::string pretty_print(const resolved_binary& e) {
    switch (e.op) {
        case binary_op::add:
        case binary_op::sub:
        case binary_op::mul:
        case binary_op::div:
        case binary_op::pow:
        case binary_op::lt:
        case binary_op::le:
        case binary_op::gt:
        case binary_op::ge:
        case binary_op::eq:
        case binary_op::ne:
        case binary_op::dot:
        case binary_op::land:
        case binary_op::lor:{
            return pretty_print(e.lhs) + to_string(e.op) + pretty_print(e.rhs);
        }
        case binary_op::min:
        case binary_op::max: {
            return to_string(e.op) + "(" + pretty_print(e.lhs) + ", " + pretty_print(e.rhs) + ")";
        }
    }
    return {};
}

std::string pretty_print(const resolved_argument& e) {
    return e.name;
}

std::string pretty_print(const resolved_variable& e) {
    return e.name;
}

std::string pretty_print(const resolved_field_access& e) {
    return pretty_print(e.object) + "." + e.field;
}

std::string pretty_print(const r_expr & e) {
    return std::visit([&](auto&& c){return pretty_print(c);}, *e);
}

// r_type
std::string pretty_print(const resolved_quantity& q) {
    return to_string(q.type);
}
std::string pretty_print(const resolved_boolean& q) {
    return "bool";
}

std::string pretty_print(const resolved_record& q) {

    std::string str = "{";
    bool first = true;
    for (const auto& f: q.fields) {
        if (!first) str += " ";
        str += f.first + ":" + pretty_print(f.second) + ";";
        first = false;
    }
    return str + "}";
}
std::string pretty_print(const r_type& q) {
    return std::visit([&](auto&& c){return pretty_print(c);}, *q);
}

// r_expr expand

std::string expand(const resolved_parameter& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');

    std::string str = single_indent + "(parameter " + e.name + "\n";
    str += expand(e.value,  indent+1);
    return str + ")";
}

std::string expand(const resolved_constant& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');

    std::string str = single_indent + "(constant " + e.name + "\n";
    str += expand(e.value, indent+1);
    return str + ")";
}

std::string expand(const resolved_state& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    return single_indent + "(state " + e.name + ")";
}

std::string expand(const resolved_record_alias& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    return single_indent + "(record_alias " + e.name + ")";
}

std::string expand(const resolved_function& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(function " + e.name +  " (\n";

    bool first = true;
    for (const auto& f: e.args) {
        if (!first) str += "\n";
        str += expand(f, indent+1);
        first = false;
    }
    return str + ")\n" + expand(e.body, indent+1) + ")";
}

std::string expand(const resolved_bind& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(bind " + e.name + " " + to_string(e.bind);
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    return str + ")";
}

std::string expand(const resolved_initial& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(initial\n";
    str += expand(e.identifier, indent+1) + "\n";
    str += expand(e.value, indent+1);
    return str + ")";
}

std::string expand(const resolved_on_event& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(on_event\n";
    str += expand(e.argument, indent+1) + "\n";
    str += expand(e.identifier, indent+1) + "\n";
    str += expand(e.value, indent+1);
    return str + ")";
}

std::string expand(const resolved_evolve& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(evolve\n";
    str += expand(e.identifier, indent+1) + "\n";
    str += expand(e.value, indent+1);
    return str + ")";
}

std::string expand(const resolved_effect& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(effect " + to_string(e.effect);
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    str += expand(e.value, indent+1);
    return str + ")";
}

std::string expand(const resolved_export& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(export\n";
    str += expand(e.identifier, indent+1);
    return str + ")";
}

std::string expand(const resolved_call& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(call " + e.f_identifier;
    for (const auto& f: e.call_args) {
        str += "\n" + expand(f, indent+1);
    }
    return str + ")";
}

std::string expand(const resolved_object& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(object\n";
    bool first = true;
    for (unsigned i = 0; i < e.record_fields.size(); ++i) {
        if (!first) str += "\n";
        str += expand(e.record_fields[i], indent + 1);
        first = false;
    }
    return str + ")";
}

std::string expand(const resolved_let& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(let\n";
    str += expand(e.identifier, indent+1) + "\n";
    str += expand(e.body, indent+1);
    return str + ")";
}

std::string expand(const resolved_conditional& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(conditional\n";
    str += expand(e.condition, indent+1) + "\n";
    str += expand(e.value_true, indent+1) + "\n";
    str += expand(e.value_false, indent+1);
    return str + ")";
}

std::string expand(const resolved_float& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');

    // get accurate double
    std::ostringstream os;
    os << std::setprecision(std::numeric_limits<double>::max_digits10) << e.value;

    return single_indent + "(" + os.str() + ")";
}

std::string expand(const resolved_int& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    return single_indent + "(" + std::to_string(e.value) + ")";
}

std::string expand(const resolved_unary& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(" + to_string(e.op) + "\n";
    str += expand(e.arg, indent+1);
    return str + ")";
}

std::string expand(const resolved_binary& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    std::string str = single_indent + "(" + to_string(e.op) + "\n";
    str += expand(e.lhs, indent+1) + "\n";
    str += expand(e.rhs, indent+1);
    return str + ")";
}

std::string expand(const resolved_argument& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    return single_indent + "(argument " + e.name + ")";
}

std::string expand(const resolved_variable& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    return single_indent + "(variable " + e.name + "\n" + expand(e.value, indent+1) + ")";
}

std::string expand(const resolved_field_access& e, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(access\n";
    str += expand(e.object, indent+1);
    str += "\n" + double_indent + "(" + e.field + ")";
    return str + ")";
}

std::string expand(const r_expr & e, int indent) {
    return std::visit([&](auto&& c){return expand(c, indent);}, *e);
}

} // namespace al
