#include <string>

#include <arblang/raw_expressions.hpp>

namespace al {
namespace raw_ir {
// module_expr
std::string module_expr::to_string() const {
    std::string str = name + "\n";
    for (const auto& p: parameters) {
        std::visit([&](auto&& arg){ str+= arg->to_string();}, p);
    }
}

// parameter_expr
std::string parameter_expr::to_string() const {
    return "";
}

// constant_expr
std::string constant_expr::to_string() const {
    return "";
}

// record_expr
std::string record_expr::to_string() const {
    return "";
}

// function_expr
std::string function_expr::to_string() const {
    return "";
}

// import_expr
std::string import_expr::to_string() const {
    return "";
}

// call_expr
std::string call_expr::to_string() const {
    return "";
}

// field_expr
std::string field_expr::to_string() const {
    return "";
}

// let_expr
std::string let_expr::to_string() const {
    return "";
}

// conditional_expr
std::string conditional_expr::to_string() const {
    return "";
}

// identifier_expr
std::string identifier_expr::to_string() const {
    return "";
}

// float_expr
std::string float_expr::to_string() const {
    return "";
}

// int_expr
std::string int_expr::to_string() const {
    return "";
}

// unary_expr
bool unary_expr::is_boolean () const {
    return op == unary_op::lnot;
}

std::string unary_expr::to_string() const {
    return "";
}

// binary_expr
bool binary_expr::is_boolean () const {
    return (op == binary_op::land) || (op == binary_op::lor) || (op == binary_op::ge) ||
           (op == binary_op::gt)   || (op == binary_op::le)  || (op == binary_op::lt) ||
           (op == binary_op::eq)   || (op == binary_op::ne);
}

std::string binary_expr::to_string() const {
    return "";
}

} // namespace al
} // namespace raw_ir
