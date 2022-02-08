#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <variant>

#include <arblang/common.hpp>
#include <arblang/token.hpp>
#include <arblang/raw_expressions.hpp>
#include <arblang/resolved_types.hpp>

namespace al {
namespace resolved_ir {

using namespace t_resolved_ir;

struct resolved_mechanism;
struct resolved_parameter;
struct resolved_constant;
struct resolved_state;
struct resolved_record_alias;
struct resolved_function;
struct resolved_argument;
struct resolved_bind;
struct resolved_initial;
struct resolved_evolve;
struct resolved_effect;
struct resolved_export;
struct resolved_call;
struct resolved_object;
struct resolved_let;
struct resolved_with;
struct resolved_conditional;
struct resolved_float;
struct resolved_int;
struct resolved_unary;
struct resolved_binary;

using resolved_expr = std::variant<
    resolved_parameter,
    resolved_constant,
    resolved_state,
    resolved_record_alias,
    resolved_function,
    resolved_argument,
    resolved_bind,
    resolved_initial,
    resolved_evolve,
    resolved_effect,
    resolved_export,
    resolved_call,
    resolved_object,
    resolved_let,
    resolved_with,
    resolved_conditional,
    resolved_float,
    resolved_int,
    resolved_unary,
    resolved_binary>;

using r_expr = std::shared_ptr<resolved_expr>;

struct resolved_mechanism {
    resolved_mechanism() {};

    std::string name;
    mechanism_kind kind;
    std::vector<r_expr> constants;      // expect resolved_constant
    std::vector<r_expr> parameters;     // expect resolved_parameter
    std::vector<r_expr> states;         // expect resolved_state
    std::vector<r_expr> functions;      // expect resolved_function
    std::vector<r_expr> bindings;       // expect resolved_bind
    std::vector<r_expr> initializations; // expect resolved_initial
    std::vector<r_expr> effects;        // expect effects_expr
    std::vector<r_expr> evolutions;     // expect resolved_evolve
    std::vector<r_expr> exports;        // expect resolved_export
    src_location loc;
};

// Top level parameters
struct resolved_parameter {
    std::string name;
    r_expr value;
    r_type type;
    src_location loc;

    resolved_parameter(std::string iden, r_expr value, r_type type, const src_location& loc):
        name(std::move(iden)), type(std::move(type)), value(std::move(value)), loc(loc) {};
};

// Top level constants
struct resolved_constant {
    std::string name;
    r_expr value;
    r_type type;
    src_location loc;

    resolved_constant(std::string iden, r_expr value, r_type type, const src_location& loc):
        name(std::move(iden)), type(std::move(type)), value(std::move(value)), loc(loc) {};
};

// Top level states
struct resolved_state {
    std::string name;
    r_type type;
    src_location loc;

    resolved_state(std::string iden, r_type type, const src_location& loc):
        name(std::move(iden)), type(std::move(type)), loc(loc) {};
};


// Top level bindables
struct resolved_bind {
    std::string name;
    bindable bind;
    std::optional<std::string> ion;
    r_type type;
    src_location loc;

    resolved_bind(std::string iden, bindable bind, std::optional<std::string> ion, r_type type, const src_location& loc):
            name(std::move(iden)), bind(bind), ion(std::move(ion)), type(std::move(type)), loc(loc) {};
};

// Top level record definitions
struct resolved_record_alias {
    std::string name;
    r_type type;
    src_location loc;

    resolved_record_alias(std::string name, r_type type, const src_location& loc):
        name(std::move(name)), type(std::move(type)), loc(loc) {};
};

// Top level function definitions
struct resolved_function {
    std::string name;
    std::vector<r_expr> args;
    r_expr body;
    r_type type;
    src_location loc;

    resolved_function(std::string name, std::vector<r_expr> args, r_expr body, r_type type, const src_location& loc):
        name(std::move(name)), args(std::move(args)), body(std::move(body)), type(std::move(type)), loc(loc) {};
};

// Function arguments, record fields, let-bound variables
struct resolved_argument {
    std::string name;
    r_type type;
    src_location loc;

    resolved_argument(std::string iden, r_type type, const src_location& loc):
            name(std::move(iden)), type(std::move(type)), loc(loc) {};
};

// Top level initialization
struct resolved_initial {
    r_expr identifier;
    r_expr value;
    r_type type;
    src_location loc;

    resolved_initial(r_expr iden, r_expr value, r_type type, const src_location& loc):
        identifier(std::move(iden)), value(std::move(value)), type(std::move(type)), loc(loc) {};
};

// Top level evolution
struct resolved_evolve {
    r_expr identifier;
    r_expr value;
    r_type type;
    src_location loc;

    resolved_evolve(r_expr iden, r_expr value, r_type type, const src_location& loc):
        identifier(std::move(iden)), value(std::move(value)), type(std::move(type)), loc(loc) {};
};

// Top level effects
struct resolved_effect {
    affectable effect;
    std::optional<std::string> ion;
    r_expr value;
    r_type type;
    src_location loc;

    resolved_effect(affectable effect, std::optional<std::string> ion, r_expr value, r_type type, const src_location& loc):
        effect(effect), ion(std::move(ion)), value(std::move(value)), type(std::move(type)), loc(loc) {};
};

// Top level exports
struct resolved_export {
    r_expr identifier;
    r_type type;
    src_location loc;

    resolved_export(r_expr iden, r_type type, const src_location& loc):
        identifier(std::move(iden)), type(std::move(type)), loc(loc) {};
};

// Function calls
struct resolved_call {
    r_expr f_identifier;
    std::vector<r_expr> call_args;
    r_type type;
    src_location loc;

    resolved_call(r_expr iden, std::vector<r_expr> args, r_type type, const src_location& loc):
        f_identifier(std::move(iden)), call_args(std::move(args)), type(std::move(type)), loc(loc) {};
};

// Object creation
struct resolved_object {
    std::optional<r_expr> r_identifier;
    std::vector<r_expr> record_fields;
    std::vector<r_expr> record_values;
    r_type type;
    src_location loc;

    resolved_object(std::optional<r_expr> iden, std::vector<r_expr> record_fields, std::vector<r_expr> records_vals, r_type type, const src_location& loc):
        r_identifier(std::move(iden)), record_fields(std::move(record_fields)), record_values(std::move(records_vals)), type(std::move(type)), loc(loc) {};
};

// Let bindings
struct resolved_let {
    r_expr identifier;
    r_expr value;
    r_expr body;
    r_type type;
    src_location loc;

    resolved_let() = default;
    resolved_let(r_expr iden, r_expr value, r_expr body, r_type type, const src_location& loc):
        identifier(std::move(iden)), value(std::move(value)), body(std::move(body)), type(std::move(type)), loc(loc) {};
};

// with bindings
struct resolved_with {
    r_expr value;
    r_expr body;
    r_type type;
    src_location loc;

    resolved_with(r_expr value, r_expr body, r_type type, const src_location& loc):
        value(std::move(value)), body(std::move(body)), type(std::move(type)), loc(loc) {};
};

// if/else statements
struct resolved_conditional {
    r_expr condition;
    r_expr value_true;
    r_expr value_false;
    r_type type;
    src_location loc;

    resolved_conditional(r_expr condition, r_expr val_true, r_expr val_false, r_type type, const src_location& loc):
        condition(std::move(condition)), value_true(std::move(val_true)), value_false(std::move(val_false)), type(std::move(type)), loc(loc) {};
};

// Number expression
struct resolved_float {
    double value;
    r_type type;
    src_location loc;

    resolved_float(double value, r_type type, const src_location& loc):
        value(value), type(std::move(type)), loc(loc) {};
};


// Number expression
struct resolved_int {
    int value;
    r_type type;
    src_location loc;

    resolved_int(int value, r_type type, const src_location& loc):
        value(value), type(std::move(type)), loc(loc) {};
};

// Both boolean and arithmetic operations
struct resolved_unary {
    unary_op op;
    r_expr arg;
    r_type type;
    src_location loc;

    resolved_unary(unary_op op, r_expr arg, r_type type, const src_location& loc):
        op(op), arg(std::move(arg)), type(std::move(type)), loc(loc) {};
};

// Both boolean and arithmetic operations
struct resolved_binary {
    binary_op op;
    r_expr lhs;
    r_expr rhs;
    r_type type;
    src_location loc;

    resolved_binary(binary_op op, r_expr lhs, r_expr rhs, r_type type, const src_location& loc):
        op(op), lhs(std::move(lhs)), rhs(std::move(rhs)), type(std::move(type)), loc(loc) {}
};

struct in_scope_map {
    std::unordered_map<std::string, resolved_parameter> param_map;
    std::unordered_map<std::string, resolved_constant> const_map;
    std::unordered_map<std::string, resolved_state> state_map;
    std::unordered_map<std::string, resolved_bind> bind_map;
    std::unordered_map<std::string, resolved_function> func_map;
    std::unordered_map<std::string, resolved_record_alias> rec_map;
    std::unordered_map<std::string, resolved_argument> local_map;
    std::unordered_map<std::string, r_type> type_map;
};

resolved_mechanism resolve(const raw_ir::mechanism_expr&, const in_scope_map&);
r_expr resolve(const raw_ir::expr &, const in_scope_map&);

std::string to_string(const resolved_mechanism&, bool include_type=true, int indent=0);
std::string to_string(const r_expr&, bool include_type=true, int indent=0);

r_type type_of(const r_expr&);
src_location location_of(const r_expr&);

template <typename T, typename... Args>
r_expr make_rexpr(Args&&... args) {
    return r_expr(new resolved_expr(T(std::forward<Args>(args)...)));
}

} // namespace raw_ir
} // namespace al