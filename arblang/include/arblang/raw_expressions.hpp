#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <variant>

#include <arblang/token.hpp>
#include <arblang/type_expressions.hpp>
#include <arblang/unit_expressions.hpp>

namespace al {
namespace raw_ir {

struct module_expr;
struct parameter_expr;
struct constant_expr;
struct record_alias_expr;
struct function_expr;
struct import_expr;
struct call_expr;
struct object_expr;
struct let_expr;
struct with_expr;
struct conditional_expr;
struct identifier_expr;
struct float_expr;
struct int_expr;
struct unary_expr;
struct binary_expr;

using raw_expr = std::variant<
    module_expr,
    parameter_expr,
    constant_expr,
    record_alias_expr,
    function_expr,
    import_expr,
    call_expr,
    object_expr,
    let_expr,
    with_expr,
    conditional_expr,
    identifier_expr,
    float_expr,
    int_expr,
    unary_expr,
    binary_expr>;

using expr = std::shared_ptr<raw_expr>;

enum class binary_op {
    add, sub, mul, div, pow,
    lt, le, gt, ge, eq, ne,
    land, lor, min, max, dot
};

enum class unary_op {
    exp, log, cos, sin, abs, exprelr, lnot, neg
};

// Top level module parameters
struct module_expr {
    module_expr() {};

    std::string name;
    std::vector<expr> constants;  // expect constant_expr
    std::vector<expr> parameters; // expect parameter_expr
    std::vector<expr> functions;  // expect function_expr
    std::vector<expr> records;    // expect record_alias_expr
    std::vector<expr> imports;    // expect import_expr
    src_location loc;
};

struct parameter_expr {
    expr identifier; // expect identifier_expr
    expr value;
    src_location loc;

    parameter_expr(expr iden, expr value, const src_location& loc): identifier(std::move(iden)), value(std::move(value)), loc(loc) {};
};

// Top level module constants
struct constant_expr {
    expr identifier; // expect identifier_expr
    expr value;
    src_location loc;

    constant_expr(expr iden, expr value, const src_location& loc): identifier(std::move(iden)), value(std::move(value)), loc(loc) {};
};

// Top level record definitions
struct record_alias_expr {
    std::string name;
    t_raw_ir::t_expr type;
    src_location loc;

    record_alias_expr(std::string name,t_raw_ir::t_expr type, const src_location& loc):
        name(std::move(name)), type(std::move(type)), loc(loc) {};
};

// Top level function definitions
struct function_expr {
    std::string name;
    std::vector<expr> args;   // expect identifier_expr
    std::optional<t_raw_ir::t_expr> ret;
    expr body;
    src_location loc;

    function_expr(std::string name, std::vector<expr> args, std::optional<t_raw_ir::t_expr> ret, expr body, const src_location& loc):
        name(std::move(name)), args(std::move(args)), ret(std::move(ret)), body(std::move(body)), loc(loc) {};
};

// Top level module imports
struct import_expr {
    std::string module_name;
    std::string module_alias;
    src_location loc;

    import_expr(std::string module_name, std::string module_alias, const src_location& loc):
        module_name(std::move(module_name)), module_alias(std::move(module_alias)), loc(loc) {};
};

// Function calls
struct call_expr {
    std::string function_name;
    std::vector<expr> call_args;
    src_location loc;

    call_expr(std::string iden, std::vector<expr> args, const src_location& loc):
        function_name(std::move(iden)), call_args(std::move(args)), loc(loc) {};
};

// Object creation
struct object_expr {
    std::optional<std::string> record_name;
    std::vector<expr> record_fields;
    std::vector<expr> record_values;
    src_location loc;

    object_expr(std::optional<std::string> record_name, std::vector<expr> record_fields, std::vector<expr> records_vals, const src_location& loc):
        record_name(std::move(record_name)), record_fields(std::move(record_fields)), record_values(std::move(records_vals)), loc(loc) {};
};

// Let bindings
struct let_expr {
    expr identifier; // expect identifier_expr
    expr value;
    expr body;
    src_location loc;

    let_expr(expr iden, expr value, expr body, const src_location& loc):
        identifier(std::move(iden)), value(std::move(value)), body(std::move(body)), loc(loc) {};
};

// with bindings
struct with_expr {
    std::string identifier; // expect identifier_expr
    expr body;
    src_location loc;

    with_expr(std::string iden, expr body, const src_location& loc):
            identifier(std::move(iden)), body(std::move(body)), loc(loc) {};
};

// if/else statements
struct conditional_expr {
    expr condition;
    expr value_true;
    expr value_false;
    src_location loc;

    conditional_expr(expr condition, expr val_true, expr val_false, const src_location& loc):
        condition(std::move(condition)), value_true(std::move(val_true)), value_false(std::move(val_false)), loc(loc) {};
};

// Number expression
struct float_expr {
    double value;
    std::optional<u_raw_ir::u_expr> unit;
    src_location loc;

    float_expr(double value, std::optional<u_raw_ir::u_expr> unit, const src_location& loc):
        value(value), unit(std::move(unit)), loc(loc) {};
};


// Number expression
struct int_expr {
    int value;
    std::optional<u_raw_ir::u_expr> unit;
    src_location loc;

    int_expr(int value,  std::optional<u_raw_ir::u_expr> unit, const src_location& loc):
        value(value), unit(std::move(unit)), loc(loc) {};
};

// Both boolean and arithmetic operations
struct unary_expr {
    unary_op op;
    expr value;
    src_location loc;

    unary_expr(tok t, expr value, const src_location& loc);

    bool is_boolean () const;
};

// Both boolean and arithmetic operations
struct binary_expr {
    binary_op op;
    expr lhs;
    expr rhs;
    src_location loc;

    binary_expr(tok t, expr lhs, expr rhs, const src_location& loc);

    bool is_boolean () const;
};

// Identifier name and type expression
struct identifier_expr {  // Is this needed? Can it be used directly and not via a shared pointer and a shared pointer to the variant?
    std::optional<t_raw_ir::t_expr> type;
    std::string name;
    src_location loc;

    identifier_expr(t_raw_ir::t_expr type, std::string name, src_location loc): type(type), name(name), loc(loc) {};
    identifier_expr(std::string name, src_location loc): type(std::nullopt), name(name), loc(loc) {};
};

std::ostream& operator<< (std::ostream&, const binary_op&);
std::ostream& operator<< (std::ostream&, const unary_op&);
std::ostream& operator<< (std::ostream&, const module_expr&);
std::ostream& operator<< (std::ostream&, const parameter_expr&);
std::ostream& operator<< (std::ostream&, const constant_expr&);
std::ostream& operator<< (std::ostream&, const record_alias_expr&);
std::ostream& operator<< (std::ostream&, const function_expr&);
std::ostream& operator<< (std::ostream&, const import_expr&);
std::ostream& operator<< (std::ostream&, const call_expr&);
std::ostream& operator<< (std::ostream&, const object_expr&);
std::ostream& operator<< (std::ostream&, const let_expr&);
std::ostream& operator<< (std::ostream&, const with_expr&);
std::ostream& operator<< (std::ostream&, const conditional_expr&);
std::ostream& operator<< (std::ostream&, const identifier_expr&);
std::ostream& operator<< (std::ostream&, const float_expr&);
std::ostream& operator<< (std::ostream&, const int_expr&);
std::ostream& operator<< (std::ostream&, const unary_expr&);
std::ostream& operator<< (std::ostream&, const binary_expr&);

template <typename T, typename... Args>
expr make_expr(Args&&... args) {
    return expr(new raw_expr(T(std::forward<Args>(args)...)));
}

} // namespace raw_ir
} // namespace al