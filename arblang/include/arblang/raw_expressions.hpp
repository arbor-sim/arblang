#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <variant>

#include <arblang/token.hpp>
#include <arblang/type_expressions.hpp>

namespace al {
namespace raw_ir {

struct module_expr;
struct parameter_expr;
struct constant_expr;
struct record_expr;
struct function_expr;
struct import_expr;
struct call_expr;
struct field_expr;
struct let_expr;
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
    record_expr,
    function_expr,
    import_expr,
    call_expr,
    field_expr,
    let_expr,
    conditional_expr,
    identifier_expr,
    float_expr,
    int_expr,
    unary_expr,
    binary_expr>;

using expr = std::shared_ptr<raw_expr>;
using type_expr = types::expr;

enum class binary_op {
    add, sub, mul, div, pow,
    lt, le, gt, ge, eq, ne,
    land, lor, min, max
};

enum class unary_op {
    exp, log, cos, sin, abs, exprelr, lnot, neg
};

// Top level module parameters
struct module_expr {
    module_expr() {};
    module_expr(module_expr&&) = default;

    std::string name;
    std::vector<expr> constants;  // expect constant_expr
    std::vector<expr> parameters; // expect parameter_expr
    std::vector<expr> functions;  // expect function_expr
    std::vector<expr> records;    // expect record_expr
    std::vector<expr> imports;    // expect import_expr
    src_location loc;

    inline std::string to_string() const;
};

struct parameter_expr {
    expr identifier; // expect identifier_expr
    expr value;
    src_location loc;

    parameter_expr(parameter_expr&&) = default;
    parameter_expr(expr iden, expr value, const src_location& loc): identifier(iden), value(value), loc(loc) {};

    inline std::string to_string() const;
};

// Top level module constants
struct constant_expr {
    expr identifier; // expect identifier_expr
    expr value;
    src_location loc;

    inline std::string to_string() const;
};

// Top level record definitions
struct record_expr {
    std::string name;
    std::vector<expr> fields;  // expect identifier_expr
    src_location loc;

    inline std::string to_string() const;
};

// Top level function definitions
struct function_expr {
    std::string name;
    std::vector<expr> args;   // expect identifier_expr
    std::optional<type_expr> ret;
    expr body;
    src_location loc;

    inline std::string to_string() const;
};

// Top level module imports
struct import_expr {
    std::string module_name;
    std::string module_alias;
    src_location loc;

    inline std::string to_string() const;
};

// Function calls
struct call_expr {
    std::string function_name;
    std::vector<expr> call_args;
    src_location loc;

    call_expr(call_expr&&) = default;
    call_expr(std::string iden, std::vector<expr> args, const src_location& loc):
        function_name(std::move(iden)), call_args(std::move(args)), loc(loc) {};

    inline std::string to_string() const;
};

// Record field access
struct field_expr {
    std::string record_name;
    std::string field_name;
    src_location loc;

    field_expr(field_expr&&) = default;
    field_expr(std::string iden, std::string field, const src_location& loc):
        record_name(std::move(iden)), field_name(std::move(field)), loc(loc) {};

    inline std::string to_string() const;
};

// Let bindings
struct let_expr {
    expr identifier; // expect identifier_expr
    expr value;
    expr body;
    src_location loc;

    let_expr(let_expr&&) = default;
    let_expr(expr iden, expr value, expr body, const src_location& loc):
        identifier(std::move(iden)), value(std::move(value)), body(std::move(body)), loc(loc) {};

    inline std::string to_string() const;
};

// if/else statements
struct conditional_expr {
    expr condition;
    expr value_true;
    expr value_false;
    src_location loc;

    conditional_expr(conditional_expr&&) = default;
    conditional_expr(expr condition, expr val_true, expr val_false, const src_location& loc):
        condition(std::move(condition)), value_true(std::move(val_true)), value_false(std::move(val_false)), loc(loc) {};

    inline std::string to_string() const;
};

// Number expression
struct float_expr {
    double value;
    std::string unit;
//    type_expr type; // No unit, automatically convert to SI/local arbor unit.
    src_location loc;

    float_expr(float_expr&&) = default;
    float_expr(double value, std::string unit, const src_location& loc):
        value(value), unit(std::move(unit)), loc(loc) {};

    inline std::string to_string() const;
};


// Number expression
struct int_expr {
    int value;
    std::string unit;
//    type_expr type; // No unit, automatically convert to SI/local arbor unit.
    src_location loc;

    int_expr(int_expr&&) = default;
    int_expr(int value, std::string unit, const src_location& loc):
            value(value), unit(std::move(unit)), loc(loc) {};

    inline std::string to_string() const;
};

// Both boolean and arithmetic operations
struct unary_expr {
    unary_op op;
    expr value;
    src_location loc;

    unary_expr(unary_expr&&) = default;
    unary_expr(tok t, expr value, const src_location& loc);

    bool is_boolean () const;
    inline std::string to_string() const;
};

// Both boolean and arithmetic operations
struct binary_expr {
    binary_op op;
    expr lhs;
    expr rhs;
    src_location loc;

    binary_expr(binary_expr&&) = default;
    binary_expr(tok t, expr lhs, expr rhs, const src_location& loc);

    bool is_boolean () const;
    inline std::string to_string() const;
};

// Identifier name and type expression
struct identifier_expr {  // Is this needed? Can it be used directly and not via a shared pointer and a shared pointer to the variant?
    type_expr type;
    std::string name;
    src_location loc;

    identifier_expr(identifier_expr&&) = default;
    identifier_expr(type_expr type, std::string name, src_location loc): type(type), name(name), loc(loc) {};

    inline std::string to_string() const;
};

template <typename T, typename... Args>
expr make_expr(Args&&... args) {
    return expr(new raw_expr(T(std::forward<Args>(args)...)));
}

} // namespace raw_ir
} // namespace al