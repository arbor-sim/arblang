#include <string>
#include <sstream>

#include <fmt/core.h>

#include <arblang/printer/print_expressions.hpp>

namespace al {
namespace resolved_ir {

void print_non_trivial_expression(const r_expr& e, std::stringstream& out, const std::string& indent) {
    if (is_resolved_let(e)) {
        print_expression(e, out, indent);
    }
}

void print_expression(const resolved_record_alias& e, std::stringstream& out, const std::string& indent) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation (after resolution).");
}
void print_expression(const resolved_constant& e, std::stringstream& out, const std::string& indent) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_constant at "
                             "this stage in the compilation (after optimization).");
}

void print_expression(const resolved_function& e, std::stringstream& out, const std::string& indent) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_function at "
                             "this stage in the compilation (after inlining).");
}

void print_expression(const resolved_call& e, std::stringstream& out, const std::string& indent) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_call at "
                             "this stage in the compilation (after inlining).");
}

void print_expression(const resolved_state& e, std::stringstream& out, const std::string& indent) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_state at "
                             "this stage in the compilation (during printing prep).");
}

void print_expression(const resolved_bind& e, std::stringstream& out, const std::string& indent) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_bind at "
                             "this stage in the compilation (during printing prep).");
}

void print_expression(const resolved_export& e, std::stringstream& out, const std::string& indent) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_export at "
                             "this stage in the compilation (during printing prep).");
}

void print_expression(const resolved_field_access& e, std::stringstream& out, const std::string& indent) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_field_access at "
                             "this stage in the compilation (after printing prep).");
}

void print_expression(const resolved_parameter& e, std::stringstream& out, const std::string& indent) {
    print_non_trivial_expression(e.value, out, indent);
}

void print_expression(const resolved_initial& e, std::stringstream& out, const std::string& indent) {
    print_non_trivial_expression(e.value, out, indent);
}

void print_expression(const resolved_on_event& e, std::stringstream& out, const std::string& indent) {
    print_non_trivial_expression(e.value, out, indent);
}

void print_expression(const resolved_evolve& e, std::stringstream& out, const std::string& indent) {
    print_non_trivial_expression(e.value, out, indent);
}

void print_expression(const resolved_effect& e, std::stringstream& out, const std::string& indent) {
    print_non_trivial_expression(e.value, out, indent);
}

void print_expression(const resolved_argument& e, std::stringstream& out, const std::string& indent) {
    out << e.name;
}

void print_expression(const resolved_variable& e, std::stringstream& out, const std::string& indent) {
    out << e.name;
}

void print_expression(const resolved_object& e, std::stringstream& out, const std::string& indent) {
    // Don't do anything. We should only encounter object for the final body of a
    // let statement or nested let statements. This is handled in print_mechanism.
}

void print_expression(const resolved_let& e, std::stringstream& out, const std::string& indent) {
    auto name = e.id_name();
    auto val  = e.id_value();
    out << indent << "auto " << name << " = ";
    print_expression(val, out, indent);
    out << ";\n";

    // only print the body if it another let statement
    if (is_resolved_let(e.body)) {
        print_expression(e.body, out, indent);
    }
}

void print_expression(const resolved_conditional& e, std::stringstream& out, const std::string& indent) {
    print_expression(e.condition, out, indent);
    out << " ? ";
    print_expression(e.value_true, out, indent);
    out << " : ";
    print_expression(e.value_false, out, indent);
    out << ";\n";
}

void print_expression(const resolved_float& e, std::stringstream& out, const std::string& indent) {
    out << e.value;
}

void print_expression(const resolved_int& e, std::stringstream& out, const std::string& indent) {
    out << e.value;
}

void print_expression(const resolved_unary& e, std::stringstream& out, const std::string& indent) {
    switch (e.op) {
        case unary_op::exp:
            out << "exp(";
            print_expression(e.arg, out, indent);
            out << ")";
            break;
        case unary_op::log:
            out << "log(";
            print_expression(e.arg, out, indent);
            out << ")";
            break;
        case unary_op::cos:
            out << "cos(";
            print_expression(e.arg, out, indent);
            out << ")";
            break;
        case unary_op::sin:
            out << "sin(";
            print_expression(e.arg, out, indent);
            out << ")";
            break;
        case unary_op::abs:
            out << "abs(";
            print_expression(e.arg, out, indent);
            out << ")";
            break;
        case unary_op::exprelr:
            out << "exprelr(";
            print_expression(e.arg, out, indent);
            out << ")";
            break;
        case unary_op::lnot:
            out << "!";
            print_expression(e.arg, out, indent);
            break;
        case unary_op::neg:
            out << "-";
            print_expression(e.arg, out, indent);
            break;
    }
}

void print_expression(const resolved_binary& e, std::stringstream& out, const std::string& indent) {
    switch (e.op) {
        case binary_op::add:
            print_expression(e.lhs, out, indent);
            out << " + ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::sub:
            print_expression(e.lhs, out, indent);
            out << " - ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::mul:
            print_expression(e.lhs, out, indent);
            out << " * ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::div:
            print_expression(e.lhs, out, indent);
            out << " / ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::pow:
            out << "pow(";
            print_expression(e.lhs, out, indent);
            out << ", ";
            print_expression(e.rhs, out, indent);
            out << ")";
            break;
        case binary_op::lt:
            print_expression(e.lhs, out, indent);
            out << " < ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::le:
            print_expression(e.lhs, out, indent);
            out << " <= ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::gt:
            print_expression(e.lhs, out, indent);
            out << " > ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::ge:
            print_expression(e.lhs, out, indent);
            out << " >= ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::eq:
            print_expression(e.lhs, out, indent);
            out << " == ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::ne:
            print_expression(e.lhs, out, indent);
            out << " != ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::land:
            print_expression(e.lhs, out, indent);
            out << " && ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::lor:
            print_expression(e.lhs, out, indent);
            out << " || ";
            print_expression(e.rhs, out, indent);
            break;
        case binary_op::min:
            out << "min(";
            print_expression(e.lhs, out, indent);
            out << ", ";
            print_expression(e.lhs, out, indent);
            out << ")";
            break;
        case binary_op::max:
            out << "max(";
            print_expression(e.lhs, out, indent);
            out << ", ";
            print_expression(e.lhs, out, indent);
            out << ")";
            break;
        case binary_op::dot: break;
    }
}

void print_expression(const r_expr& e, std::stringstream& out, const std::string& indent) {
    return std::visit([&](auto&& c){return print_expression(c, out, indent);}, *e);
}

} // namespace resolved_ir
} // namespace al