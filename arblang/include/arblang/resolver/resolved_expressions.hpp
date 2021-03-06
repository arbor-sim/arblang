#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <variant>

#include <arblang/parser/token.hpp>
#include <arblang/parser/parsed_expressions.hpp>
#include <arblang/resolver/resolved_types.hpp>
#include <arblang/util/common.hpp>

namespace al {
namespace resolved_ir {

using namespace resolved_type_ir;

struct resolved_mechanism;
struct resolved_argument;
struct resolved_variable;
struct resolved_field_access;
struct resolved_parameter;
struct resolved_constant;
struct resolved_state;
struct resolved_record_alias;
struct resolved_function;
struct resolved_bind;
struct resolved_initial;
struct resolved_on_event;
struct resolved_evolve;
struct resolved_effect;
struct resolved_export;
struct resolved_call;
struct resolved_object;
struct resolved_let;
struct resolved_conditional;
struct resolved_float;
struct resolved_int;
struct resolved_unary;
struct resolved_binary;

using resolved_expr = std::variant<
    resolved_argument,
    resolved_variable,
    resolved_field_access,
    resolved_parameter,
    resolved_constant,
    resolved_state,
    resolved_record_alias,
    resolved_function,
    resolved_bind,
    resolved_initial,
    resolved_on_event,
    resolved_evolve,
    resolved_effect,
    resolved_export,
    resolved_call,
    resolved_object,
    resolved_let,
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
    std::vector<r_expr> on_events;      // expect resolved_on_events
    std::vector<r_expr> effects;        // expect effects_expr
    std::vector<r_expr> evolutions;     // expect resolved_evolve
    std::vector<r_expr> exports;        // expect resolved_export
    src_location loc;
};

// Used for function arguments and to refer to parameters, constants, states and bindings.
// For function arguments, it is a placeholder until inlining at which point, it should
// become a resolved_variable.
// For parameters, constants, states and bindings, it is a placeholder until inlining at
// which point, it should become a resolved_variable.
// For constants, it is also a placeholder until constant propagation at which point, it
// should be replaced by a resolved_float or resolved_int.
struct resolved_argument {
    std::string name; // argument name
    r_type type;
    src_location loc;

    resolved_argument(std::string iden, r_type type, const src_location& loc):
            name(std::move(iden)), type(std::move(type)), loc(loc) {};
};

// references to let bound variables.
struct resolved_variable {
    std::string name; // variable name
    r_expr value;     // pointer to the expression value
    r_type type;
    src_location loc;

    resolved_variable(std::string iden, r_expr value, r_type type, const src_location& loc):
            name(std::move(iden)), value(std::move(value)), type(std::move(type)), loc(loc) {};
};

// Used for object field access using the dot operator.
// It will either have been replaced by a resolved_variable after inlining,
// or it will remain if the object is a state.
struct resolved_field_access {
    r_expr object;   // pointer to the expression representing the object
    std::string field;  // name of the field in the tuple representing the object
    r_type type;
    src_location loc;

    resolved_field_access(r_expr object, std::string field, r_type type, const src_location& loc):
            object(std::move(object)), field(std::move(field)), type(std::move(type)), loc(loc) {};
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

// Top level initialization
struct resolved_initial {
    r_expr identifier;
    r_expr value;
    r_type type;
    src_location loc;

    resolved_initial(r_expr iden, r_expr value, r_type type, const src_location& loc):
        identifier(std::move(iden)), value(std::move(value)), type(std::move(type)), loc(loc) {};
};

// Top level on_event
struct resolved_on_event {
    r_expr argument;
    r_expr identifier;
    r_expr value;
    r_type type;
    src_location loc;

    resolved_on_event(r_expr arg, r_expr iden, r_expr value, r_type type, const src_location& loc):
        argument(std::move(arg)), identifier(std::move(iden)), value(std::move(value)), type(std::move(type)), loc(loc) {};
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
    std::string f_identifier; // keep as string, when we finally inline, resolved_calls will disappear
    std::vector<r_expr> call_args;
    r_type type;
    src_location loc;

    resolved_call(std::string iden, std::vector<r_expr> args, r_type type, const src_location& loc):
        f_identifier(std::move(iden)), call_args(std::move(args)), type(std::move(type)), loc(loc) {};
};

// Object creation
struct resolved_object {
    std::vector<r_expr> record_fields;
    r_type type;
    src_location loc;

    resolved_object(std::vector<r_expr> record_fields, r_type type, const src_location& loc):
        record_fields(std::move(record_fields)), type(std::move(type)), loc(loc) {};

    resolved_object(std::vector<std::string> names, std::vector<r_expr> values, r_type type, const src_location& loc);

    std::vector<r_expr> field_values() const;
    void field_values(std::vector<r_expr>);
    std::vector<std::string> field_names() const;
};

// Let bindings
struct resolved_let {
    r_expr identifier;
    r_expr body;
    r_type type;
    src_location loc;

    resolved_let() = default;
    resolved_let(r_expr iden, r_expr body, r_type type, const src_location& loc):
        identifier(std::move(iden)), body(std::move(body)), type(std::move(type)), loc(loc) {};

    resolved_let(std::string iden, r_expr value, r_expr body, r_type type, const src_location& loc);

    r_expr id_value() const;
    void id_value(r_expr);
    std::string id_name() const;
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

    resolved_unary(unary_op op, r_expr arg, const src_location& loc);
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

    resolved_binary(binary_op op, r_expr lhs, r_expr rhs, const src_location& loc);
};

bool operator==(const resolved_argument& lhs, const resolved_argument& rhs);
bool operator==(const resolved_variable& lhs, const resolved_variable& rhs);
bool operator==(const resolved_field_access& lhs, const resolved_field_access& rhs);
bool operator==(const resolved_parameter& lhs, const resolved_parameter& rhs);
bool operator==(const resolved_constant& lhs, const resolved_constant& rhs);
bool operator==(const resolved_state& lhs, const resolved_state& rhs);
bool operator==(const resolved_record_alias& lhs, const resolved_record_alias& rhs);
bool operator==(const resolved_function& lhs, const resolved_function& rhs);
bool operator==(const resolved_bind& lhs, const resolved_bind& rhs);
bool operator==(const resolved_initial& lhs, const resolved_initial& rhs);
bool operator==(const resolved_on_event& lhs, const resolved_on_event& rhs);
bool operator==(const resolved_evolve& lhs, const resolved_evolve& rhs);
bool operator==(const resolved_effect& lhs, const resolved_effect& rhs);
bool operator==(const resolved_export& lhs, const resolved_export& rhs);
bool operator==(const resolved_call& lhs, const resolved_call& rhs);
bool operator==(const resolved_object& lhs, const resolved_object& rhs);
bool operator==(const resolved_let& lhs, const resolved_let& rhs);
bool operator==(const resolved_conditional& lhs, const resolved_conditional& rhs);
bool operator==(const resolved_float& lhs, const resolved_float& rhs);
bool operator==(const resolved_int& lhs, const resolved_int& rhs);
bool operator==(const resolved_unary& lhs, const resolved_unary& rhs);
bool operator==(const resolved_binary& lhs, const resolved_binary& rhs);

std::string to_string(const resolved_mechanism&, bool include_type=true, bool expand_var=false, int indent=0);
std::string to_string(const r_expr&, bool include_type=true, bool expand_var=false, int indent=0);

r_type type_of(const r_expr&);
src_location location_of(const r_expr&);

std::optional<resolved_argument> is_resolved_argument(const r_expr&);
std::optional<resolved_variable> is_resolved_variable(const r_expr&);
std::optional<resolved_field_access> is_resolved_field_access(const r_expr&);
std::optional<resolved_mechanism> is_resolved_mechanism(const r_expr&);
std::optional<resolved_parameter> is_resolved_parameter(const r_expr&);
std::optional<resolved_constant> is_resolved_constant(const r_expr&);
std::optional<resolved_state> is_resolved_state(const r_expr&);
std::optional<resolved_record_alias> is_resolved_record_alias(const r_expr&);
std::optional<resolved_function> is_resolved_function(const r_expr&);
std::optional<resolved_bind> is_resolved_bind(const r_expr&);
std::optional<resolved_initial> is_resolved_initial(const r_expr&);
std::optional<resolved_on_event> is_resolved_on_event(const r_expr&);
std::optional<resolved_evolve> is_resolved_evolve(const r_expr&);
std::optional<resolved_effect> is_resolved_effect(const r_expr&);
std::optional<resolved_export> is_resolved_export(const r_expr&);
std::optional<resolved_call> is_resolved_call(const r_expr&);
std::optional<resolved_object> is_resolved_object(const r_expr&);
std::optional<resolved_let> is_resolved_let(const r_expr&);
std::optional<resolved_conditional> is_resolved_conditional(const r_expr&);
std::optional<resolved_float> is_resolved_float(const r_expr&);
std::optional<resolved_int> is_resolved_int(const r_expr&);
std::optional<resolved_unary> is_resolved_unary(const r_expr&);
std::optional<resolved_binary> is_resolved_binary(const r_expr&);

template <typename T, typename... Args>
r_expr make_rexpr(Args&&... args) {
    return r_expr(new resolved_expr(T(std::forward<Args>(args)...)));
}

} // namespace parsed_ir
} // namespace al