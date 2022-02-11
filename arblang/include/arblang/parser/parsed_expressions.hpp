#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <variant>

#include <arblang/parser/token.hpp>
#include <arblang/parser/parsed_types.hpp>
#include <arblang/parser/parsed_units.hpp>
#include <arblang/util/common.hpp>

namespace al {
namespace parsed_ir {

struct parsed_mechanism;
struct parsed_parameter;
struct parsed_constant;
struct parsed_state;
struct parsed_record_alias;
struct parsed_function;
struct parsed_bind;
struct parsed_initial;
struct parsed_evolve;
struct parsed_effect;
struct parsed_export;
struct parsed_call;
struct parsed_object;
struct parsed_let;
struct parsed_with;
struct parsed_conditional;
struct parsed_identifier;
struct parsed_float;
struct parsed_int;
struct parsed_unary;
struct parsed_binary;

using parsed_expr = std::variant<
    parsed_parameter,
    parsed_constant,
    parsed_state,
    parsed_record_alias,
    parsed_function,
    parsed_bind,
    parsed_initial,
    parsed_evolve,
    parsed_effect,
    parsed_export,
    parsed_call,
    parsed_object,
    parsed_let,
    parsed_with,
    parsed_conditional,
    parsed_identifier,
    parsed_float,
    parsed_int,
    parsed_unary,
    parsed_binary>;

using p_expr = std::shared_ptr<parsed_expr>;

struct parsed_mechanism {
    parsed_mechanism() = default;;

    std::string name;
    mechanism_kind kind;
    std::vector<p_expr> constants;      // expect parsed_constant
    std::vector<p_expr> parameters;     // expect parsed_parameter
    std::vector<p_expr> states;         // expect parsed_state
    std::vector<p_expr> functions;      // expect parsed_function
    std::vector<p_expr> records;        // expect parsed_record_alias
    std::vector<p_expr> bindings;       // expect parsed_bind
    std::vector<p_expr> initializations; // expect parsed_initial
    std::vector<p_expr> effects;        // expect effects_expr
    std::vector<p_expr> evolutions;     // expect parsed_evolve
    std::vector<p_expr> exports;        // expect parsed_export
    src_location loc;

    bool set_kind(tok t);
};

// Top level parameters
struct parsed_parameter {
    p_expr identifier; // expect parsed_identifier
    p_expr value;
    src_location loc;

    parsed_parameter(p_expr iden, p_expr value, const src_location& loc): identifier(std::move(iden)), value(std::move(value)), loc(loc) {};
};

// Top level constants
struct parsed_constant {
    p_expr identifier; // expect parsed_identifier
    p_expr value;
    src_location loc;

    parsed_constant(p_expr iden, p_expr value, const src_location& loc): identifier(std::move(iden)), value(std::move(value)), loc(loc) {};
};

// Top level states
struct parsed_state {
    p_expr identifier; // expect parsed_identifier
    src_location loc;

    parsed_state(p_expr iden,const src_location& loc): identifier(std::move(iden)), loc(loc) {};
};

// Top level record definitions
struct parsed_record_alias {
    std::string name;
    parsed_type_ir::p_type type;
    src_location loc;

    parsed_record_alias(std::string name, parsed_type_ir::p_type type, const src_location& loc):
        name(std::move(name)), type(std::move(type)), loc(loc) {};
};

// Top level function definitions
struct parsed_function {
    std::string name;
    std::vector<p_expr> args;   // expect parsed_identifier
    std::optional<parsed_type_ir::p_type> ret;
    p_expr body;
    src_location loc;

    parsed_function(std::string name, std::vector<p_expr> args, std::optional<parsed_type_ir::p_type> ret, p_expr body, const src_location& loc):
        name(std::move(name)), args(std::move(args)), ret(std::move(ret)), body(std::move(body)), loc(loc) {};
};

// Top level bindables
struct parsed_bind {
    p_expr identifier; // expect parsed_identifier
    bindable bind;
    std::optional<std::string> ion;
    src_location loc;

    parsed_bind(p_expr iden, const token& t, const std::string& ion, const src_location& loc);
};

// Top level initialization
struct parsed_initial {
    p_expr identifier; // expect parsed_identifier
    p_expr value;
    src_location loc;

    parsed_initial(p_expr iden, p_expr value, const src_location& loc): identifier(std::move(iden)), value(std::move(value)), loc(loc) {};
};

// Top level evolution
struct parsed_evolve {
    p_expr identifier; // expect parsed_identifier
    p_expr value;
    src_location loc;

    parsed_evolve(p_expr iden, p_expr value, const src_location& loc): identifier(std::move(iden)), value(std::move(value)), loc(loc) {};
};

// Top level effects
struct parsed_effect {
    affectable effect;
    std::optional<std::string> ion;
    std::optional<parsed_type_ir::p_type> type;
    p_expr value;
    src_location loc;

    parsed_effect(const token& t, const std::string& ion, std::optional<parsed_type_ir::p_type> type, p_expr value, const src_location& loc);
    parsed_effect(affectable effect, std::optional<std::string> ion, std::optional<parsed_type_ir::p_type> type, p_expr value, const src_location& loc):
        effect(std::move(effect)), ion(std::move(ion)), type(std::move(type)), value(std::move(value)), loc(loc) {};
};

// Top level exports
struct parsed_export {
    p_expr identifier; // expect parsed_identifier
    src_location loc;

    parsed_export(p_expr iden, const src_location& loc): identifier(std::move(iden)), loc(loc) {};
};

// Function calls
struct parsed_call {
    std::string function_name;
    std::vector<p_expr> call_args;
    src_location loc;

    parsed_call(std::string iden, std::vector<p_expr> args, const src_location& loc):
        function_name(std::move(iden)), call_args(std::move(args)), loc(loc) {};
};

// Object creation
struct parsed_object {
    std::optional<std::string> record_name;
    std::vector<p_expr> record_fields;
    std::vector<p_expr> record_values;
    src_location loc;

    parsed_object(std::optional<std::string> record_name, std::vector<p_expr> record_fields, std::vector<p_expr> records_vals, const src_location& loc):
        record_name(std::move(record_name)), record_fields(std::move(record_fields)), record_values(std::move(records_vals)), loc(loc) {};
    std::string to_string(int indent=0);
};

// Let bindings
struct parsed_let {
    p_expr identifier; // expect parsed_identifier
    p_expr value;
    p_expr body;
    src_location loc;

    parsed_let() = default;
    parsed_let(p_expr iden, p_expr value, p_expr body, const src_location& loc):
        identifier(std::move(iden)), value(std::move(value)), body(std::move(body)), loc(loc) {};
};

// with bindings
struct parsed_with {
    p_expr value;
    p_expr body;
    src_location loc;

    parsed_with(p_expr value, p_expr body, const src_location& loc):
            value(std::move(value)), body(std::move(body)), loc(loc) {};
};

// if/else statements
struct parsed_conditional {
    p_expr condition;
    p_expr value_true;
    p_expr value_false;
    src_location loc;

    parsed_conditional(p_expr condition, p_expr val_true, p_expr val_false, const src_location& loc):
        condition(std::move(condition)), value_true(std::move(val_true)), value_false(std::move(val_false)), loc(loc) {};
};

// Number expression
struct parsed_float {
    double value;
    parsed_unit_ir::p_unit unit;
    src_location loc;

    parsed_float(double value, parsed_unit_ir::p_unit unit, const src_location& loc):
        value(value), unit(std::move(unit)), loc(loc) {};
};


// Number expression
struct parsed_int {
    long int value;
    parsed_unit_ir::p_unit unit;
    src_location loc;

    parsed_int(int value,  parsed_unit_ir::p_unit unit, const src_location& loc):
        value(value), unit(std::move(unit)), loc(loc) {};
};

// Both boolean and arithmetic operations
struct parsed_unary {
    unary_op op;
    p_expr value;
    src_location loc;

    parsed_unary(tok t, p_expr value, const src_location& loc);
    parsed_unary(unary_op op, p_expr value, const src_location& loc):
        op(op), value(std::move(value)), loc(loc) {};
    bool is_boolean () const;
};

// Both boolean and arithmetic operations
struct parsed_binary {
    binary_op op;
    p_expr lhs;
    p_expr rhs;
    src_location loc;

    parsed_binary(tok t, p_expr lhs, p_expr rhs, const src_location& loc);
    parsed_binary(binary_op op, p_expr lhs, p_expr rhs, const src_location& loc):
        op(op), lhs(std::move(lhs)), rhs(std::move(rhs)), loc(loc) {};
    bool is_boolean () const;
};

// Identifier name and type expression
struct parsed_identifier {
    std::optional<parsed_type_ir::p_type> type;
    std::string name;
    src_location loc;

    parsed_identifier(parsed_type_ir::p_type type, std::string name, src_location loc): type(type), name(name), loc(loc) {};
    parsed_identifier(std::string name, src_location loc): type(std::nullopt), name(name), loc(loc) {};
};

// to_string
std::string to_string(const parsed_mechanism&, int indent=0);
std::string to_string(const p_expr&, int indent=0);

// get location
src_location location_of(const p_expr&);

template <typename T, typename... Args>
p_expr make_pexpr(Args&&... args) {
    return p_expr(new parsed_expr(T(std::forward<Args>(args)...)));
}

} // namespace parsed_ir
} // namespace al