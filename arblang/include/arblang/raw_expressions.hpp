#include <memory>
#include <string>
#include <vector>
#include <variant>

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
    add, aub, mul, div,
    lt, le, gt, ge, eq, ne,
    land, lor
};

enum class unary_op {
    exp, log, cos, sin, abs, exprelr, lnot
};

// Identifier name and type expression
struct identifier_expr {
    std::string name;
    type_expr type;
};

struct module_expr {
    std::string name;
    std::vector<constant_expr> constants;
    std::vector<parameter_expr> parameters;
    std::vector<function_expr> functions;
    std::vector<record_expr> records;
    std::vector<import_expr> imports;
};

// Top level module parameters
struct parameter_expr {
    identifier_expr identifier;
    expr value;
};

// Top level module constants
struct constant_expr {
    identifier_expr identifier;
    expr value;
};

// Top level record definitions
struct record_expr {
    std::string name;
    std::vector<identifier_expr> fields;
};

// Top level function definitions
struct function_expr {
    std::string name;
    std::vector<identifier_expr> args;
    std::optional<type_expr> ret;
    expr body;
};

// Top level module imports
struct import_expr {
    std::string module_name;
    std::string module_alias;
};

// Function calls
struct call_expr {
    std::string function_name;
    std::vector<expr> call_args;
};

// Record field access
struct field_expr {
    std::string record_name;
    std::string field_name;
};

// Let bindings
struct let_expr {
    identifier_expr identifier;
    expr value;
    expr body;
};

// if/else statements
struct conditional_expr {
    expr condition;
    expr value_true;
    expr value_false;
};

// Number expression
struct float_expr {
    double value;
    type_expr type; // No unit, automatically convert to SI/local arbor unit.
};


// Number expression
struct int_expr {
    int value;
    type_expr type; // No unit, automatically convert to SI/local arbor unit.
};

// Both boolean and arithmetic operations
struct unary_expr {
    unary_op opr;
    expr value;
};

// Both boolean and arithmetic operations
struct binary_expr {
    binary_op op;
    expr lhs;
    expr rhs;
};

} // namespace raw_ir
} // namespace al