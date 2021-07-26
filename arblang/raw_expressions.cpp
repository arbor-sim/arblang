#include <cassert>
#include <optional>
#include <sstream>
#include <string>

#include <arblang/raw_expressions.hpp>

namespace al {
namespace raw_ir {
std::optional<binary_op> gen_binary_op(tok t) {
    switch (t) {
        case tok::plus:     return binary_op::add;
        case tok::minus:    return binary_op::sub;
        case tok::times:    return binary_op::mul;
        case tok::divide:   return binary_op::div;
        case tok::pow:      return binary_op::pow;
        case tok::ne:       return binary_op::ne;
        case tok::lt:       return binary_op::lt;
        case tok::le:       return binary_op::le;
        case tok::gt:       return binary_op::gt;
        case tok::ge:       return binary_op::ge;
        case tok::land:     return binary_op::land;
        case tok::lor:      return binary_op::lor;
        case tok::equality: return binary_op::eq;
        case tok::max:      return binary_op::max;
        case tok::min:      return binary_op::min;
        default: return {};
    }
}
std::optional<unary_op> gen_unary_op(tok t) {
    switch (t) {
        case tok::exp:     return unary_op::exp;
        case tok::exprelr: return unary_op::exprelr;
        case tok::log:     return unary_op::log;
        case tok::cos:     return unary_op::cos;
        case tok::sin:     return unary_op::sin;
        case tok::abs:     return unary_op::abs;
        case tok::lnot:    return unary_op::lnot;
        case tok::minus:   return unary_op::neg;
        default: return {};
    }
}

std::ostream& operator<< (std::ostream& o, const binary_op& op) {
    switch (op) {
        case binary_op::add:  return o << "+";
        case binary_op::sub:  return o << "-";
        case binary_op::mul:  return o << "*";
        case binary_op::div:  return o << "/";
        case binary_op::pow:  return o << "^";
        case binary_op::ne:   return o << "!=";
        case binary_op::lt:   return o << "<";
        case binary_op::le:   return o << "<=";
        case binary_op::gt:   return o << ">";
        case binary_op::ge:   return o << ">=";
        case binary_op::land: return o << "&&";
        case binary_op::lor:  return o << "||";
        case binary_op::eq:   return o << "==";
        case binary_op::max:  return o << "max";
        case binary_op::min:  return o << "min";
        default: return o;
    }
}
std::ostream& operator<< (std::ostream& o, const unary_op& op) {
    switch (op) {
        case unary_op::exp:     return o << "exp";
        case unary_op::exprelr: return o << "exprelr";
        case unary_op::log:     return o << "log";
        case unary_op::cos:     return o << "cos";
        case unary_op::sin:     return o << "sin";
        case unary_op::abs:     return o << "abs";
        case unary_op::lnot:    return o << "lnot";
        case unary_op::neg:     return o << "-";
        default: return o;
    }
}

// module_expr
std::ostream& operator<< (std::ostream& o, const module_expr& e) {
    o << "(module_expr " << e.name << "\n";
    for (const auto& p: e.parameters) {
        std::visit([&](auto&& c){o << c << "\n";}, *p);
    }
    for (const auto& p: e.constants) {
        std::visit([&](auto&& c){o << c << "\n";}, *p);
    }
    for (const auto& p: e.functions) {
        std::visit([&](auto&& c){o << c << "\n";}, *p);
    }
    for (const auto& p: e.records) {
        std::visit([&](auto&& c){o << c << "\n";}, *p);
    }
    for (const auto& p: e.imports) {
        std::visit([&](auto&& c){o << c << "\n";}, *p);
    }
    return o << ")";
}

// parameter_expr
std::ostream& operator<< (std::ostream& o, const parameter_expr& e) {
    o << "(parameter_expr ";
    std::visit([&](auto&& c){o << c << " ";}, *e.identifier);
    std::visit([&](auto&& c){o << c << " ";}, *e.value);
    return o << e.loc << ")";
}

// constant_expr
std::ostream& operator<< (std::ostream& o, const constant_expr& e) {
    o << "(constant_expr ";
    std::visit([&](auto&& c){o << c << " ";}, *e.identifier);
    std::visit([&](auto&& c){o << c << " ";}, *e.value);
    return o << e.loc << ")";
}

// record_expr
std::ostream& operator<< (std::ostream& o, const record_expr& e) {
    assert(e.fields.size() == e.init_values.size());
    o << "(record_expr " << e.name <<  " ";
    for (unsigned i = 0; i < e.fields.size(); ++i) {
        std::visit([&](auto&& c){o << "(" << c << " ";}, *(e.fields[i]));
        if (e.init_values[i]) {
            std::visit([&](auto&& c){o << c << ") ";}, *(e.init_values[i].value()));
        }
    }
    return o << e.loc << ")";
}

// function_expr
std::ostream& operator<< (std::ostream& o, const function_expr& e) {
    o << "(function_expr " << e.name <<  " ";
    for (const auto& f: e.args) {
        std::visit([&](auto&& c){o << c << " ";}, *f);
    }
    if (e.ret) {
        std::visit([&](auto&& c){o << c << " ";}, *(e.ret.value()));
    }
    std::visit([&](auto&& c){o << c << " ";}, *e.body);
    return o << e.loc << ")";
}

// import_expr
std::ostream& operator<< (std::ostream& o, const import_expr& e) {
    return o << "import_expr " << e.module_name <<  " " << e.module_alias << " " << e.loc << ")";
}

// call_expr
std::ostream& operator<< (std::ostream& o, const call_expr& e) {
    o << "(call_expr " << e.function_name <<  " ";
    for (const auto& f: e.call_args) {
        std::visit([&](auto&& c){o << c << " ";}, *f);
    }
    return o << e.loc << ")";
}

// object_expr
std::ostream& operator<< (std::ostream& o, const object_expr& e) {
    assert(e.record_fields.size() == e.record_values.size());
    o << "(object_expr " << e.record_name <<  " ";
    for (unsigned i = 0; i < e.record_fields.size(); ++i) {
        std::visit([&](auto&& c){o << "(" << c << " ";}, *(e.record_fields[i]));
        std::visit([&](auto&& c){o << c << ") ";}, *(e.record_values[i]));
    }
    return o << e.loc << ")";
}

// field_expr
std::ostream& operator<< (std::ostream& o, const field_expr& e) {
    return o << "(field_expr " << e.record_name << " " << e.field_name << " " << e.loc << ")";
}

// let_expr
std::ostream& operator<< (std::ostream& o, const let_expr& e) {
    o << "(let_expr ";
    std::visit([&](auto&& c){o << c << " ";}, *e.identifier);
    std::visit([&](auto&& c){o << c << " ";}, *e.value);
    std::visit([&](auto&& c){o << c << " ";}, *e.body);
    return o << e.loc << ")";
}

// conditional_expr
std::ostream& operator<< (std::ostream& o, const conditional_expr& e) {
    o << "(conditional_expr ";
    std::visit([&](auto&& c){o << c << " ";}, *e.condition);
    std::visit([&](auto&& c){o << c << " ";}, *e.value_true);
    std::visit([&](auto&& c){o << c << " ";}, *e.value_false);
    return o << e.loc << ")";
}

// identifier_expr
std::ostream& operator<< (std::ostream& o, const identifier_expr& e) {
    o << "(identifier_expr " << e.name << " ";
    if (e.type) {
        std::visit([&](auto&& c) { o << c << " "; }, *(e.type.value()));
    }
    return o << e.loc << ")";
}

// float_expr
std::ostream& operator<< (std::ostream& o, const float_expr& e) {
    return o << "(float_expr " << e.value << " " << e.unit << " " << e.loc << ")";
}

// int_expr
std::ostream& operator<< (std::ostream& o, const int_expr& e) {
    return o << "(int_expr " << e.value << " " << e.unit << " " << e.loc << ")";
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

std::ostream& operator<< (std::ostream& o, const unary_expr& e) {
    o << "(unary_expr " << e.op << " ";
    std::visit([&](auto&& c){o << c << " ";}, *e.value);
    return o << e.loc << ")";
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

std::ostream& operator<< (std::ostream& o, const binary_expr& e) {
    o << "(binary_expr " << e.op << " ";
    std::visit([&](auto&& c){o << c << " ";}, *e.lhs);
    std::visit([&](auto&& c){o << c << " ";}, *e.rhs);
    return o << e.loc << ")";
}

} // namespace al
} // namespace raw_ir
