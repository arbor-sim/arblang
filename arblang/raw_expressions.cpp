#include <optional>
#include <sstream>
#include <string>

#include <arblang/raw_expressions.hpp>

namespace al {
namespace raw_ir {
std::optional<binary_op> gen_binary_op(tok t) {
    switch (t) {
        case tok::plus:
            return binary_op::add;
        case tok::minus:
            return binary_op::sub;
        case tok::times:
            return binary_op::mul;
        case tok::divide:
            return binary_op::div;
        case tok::pow:
            return binary_op::pow;
        case tok::ne:
            return binary_op::ne;
        case tok::lt:
            return binary_op::lt;
        case tok::le:
            return binary_op::le;
        case tok::gt:
            return binary_op::gt;
        case tok::ge:
            return binary_op::ge;
        case tok::land:
            return binary_op::land;
        case tok::lor:
            return binary_op::lor;
        case tok::equality:
            return binary_op::eq;
        case tok::max:
            return binary_op::max;
        case tok::min:
            return binary_op::min;
        default: return {};
    }
}
std::optional<unary_op> gen_unary_op(tok t) {
    switch (t) {
        case tok::exp:
            return unary_op::exp;
        case tok::exprelr:
            return unary_op::exprelr;
        case tok::log:
            return unary_op::log;
        case tok::cos:
            return unary_op::cos;
        case tok::sin:
            return unary_op::sin;
        case tok::abs:
            return unary_op::abs;
        case tok::lnot:
            return unary_op::lnot;
        case tok::minus:
            return unary_op::neg;
        default: return {};
    }
}

// module_expr
std::string module_expr::to_string() const {
    std::string str = name + "\n";
    for (const auto& p: parameters) {
        std::visit([&](auto&& arg){ str+= arg.to_string();}, *p);
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
unary_expr::unary_expr(tok t, expr value, const src_location& loc):
    value(std::move(value)), loc(loc) {
    if (auto uop = gen_unary_op(t)) {
        op = uop.value();
    } else {
        throw std::runtime_error("Unexpected unary operator token");
    };
}

bool unary_expr::is_boolean () const {
    return op == unary_op::lnot;
}

std::string unary_expr::to_string() const {
    return "";
}

// binary_expr
binary_expr::binary_expr(tok t, expr lhs, expr rhs, const src_location& loc):
    lhs(std::move(lhs)), rhs(std::move(rhs)), loc(loc) {
    if (auto bop = gen_binary_op(t)) {
        op = bop.value();
    } else {
        throw std::runtime_error("Unexpected binary operator token");
    };
}

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
