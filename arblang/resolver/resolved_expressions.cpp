#include <iomanip>
#include <cassert>
#include <string>
#include <utility>
#include <variant>

#include <fmt/core.h>

#include <arblang/resolver/resolved_expressions.hpp>
#include <arblang/resolver/resolved_types.hpp>
#include <arblang/util/common.hpp>

namespace al {
namespace resolved_ir {
using namespace resolved_type_ir;

resolved_let::resolved_let(std::string iden, r_expr value, r_expr body, r_type type, const src_location& loc):
    body(std::move(body)), type(std::move(type)), loc(loc)
{
    identifier = make_rexpr<resolved_variable>(iden, value, type_of(value), loc);
}

r_expr resolved_let::id_value() const {
    if (auto id = std::get_if<resolved_variable>(identifier.get())) {
        return id->value;
    }
    throw std::runtime_error("internal compiler error: expected resolved_variable at " + to_string(loc));
}

void resolved_let::id_value(r_expr val) {
    if (auto id = std::get_if<resolved_variable>(identifier.get())) {
        id->value = std::move(val);
    } else {
        throw std::runtime_error("internal compiler error: expected resolved_variable at " + to_string(loc));
    }
}

std::string resolved_let::id_name() const {
    if (auto id = std::get_if<resolved_variable>(identifier.get())) {
        return id->name;
    }
    throw std::runtime_error("internal compiler error: expected resolved_variable at " + to_string(loc));
}

resolved_object::resolved_object(std::vector<std::string> names,
                                 std::vector<r_expr> values,
                                 r_type type,
                                 const src_location& loc):
     type(std::move(type)), loc(loc)
{
    assert(names.size() == values.size());
    for (unsigned i = 0; i < names.size(); ++i) {
        record_fields.push_back(
                make_rexpr<resolved_variable>(
                        std::move(names[i]), std::move(values[i]), type_of(values[i]), location_of(values[i])));
    }
}

std::vector<r_expr> resolved_object::field_values() const {
    std::vector<r_expr> vals;
    for (const auto& field: record_fields) {
        if (auto id = std::get_if<resolved_variable>(field.get())) {
            vals.push_back(id->value);
        } else {
            throw std::runtime_error("internal compiler error: expected resolved_variable at " + to_string(loc));
        }
    }
    return vals;
}

void resolved_object::field_values(std::vector<r_expr> vals) {
    for (unsigned i = 0; i < record_fields.size(); ++i) {
        auto& field = record_fields[i];
        if (auto id = std::get_if<resolved_variable>(field.get())) {
            id->value = std::move(vals[i]);
        } else {
            throw std::runtime_error("internal compiler error: expected resolved_variable at " + to_string(loc));
        }
    }
}

std::vector<std::string> resolved_object::field_names() const {
    std::vector<std::string> names;
    for (const auto& field: record_fields) {
        if (auto id = std::get_if<resolved_variable>(field.get())) {
            names.push_back(id->name);
        } else {
            throw std::runtime_error("internal compiler error: expected resolved_variable at " + to_string(loc));
        }
    }
    return names;
}

resolved_binary::resolved_binary(binary_op op, r_expr l, r_expr r, const src_location& loc):
    op(op), lhs(std::move(l)), rhs(std::move(r)), loc(loc)
{
    auto lhs_q = std::get_if<resolved_quantity>(type_of(lhs).get());
    auto rhs_q = std::get_if<resolved_quantity>(type_of(rhs).get());

    auto lhs_b = std::get_if<resolved_boolean>(type_of(lhs).get());
    auto rhs_b = std::get_if<resolved_boolean>(type_of(rhs).get());

    bool is_bool = (lhs_b && rhs_b);
    bool is_quantity = (lhs_q && rhs_q);

    auto incompatible_op = [&]() {
        throw std::runtime_error(fmt::format("Internal compiler error: cannot apply operand {} to types {} and {} at {}",
                                             to_string(op), to_string(type_of(lhs)), to_string(type_of(rhs)), to_string(loc)));
    };
    auto incompatible_args = [&]() {
        throw std::runtime_error(fmt::format("Internal compiler error: binary operand {} lhs and rhs types "
                                             "don't match at {}", to_string(op), to_string(loc)));
    };

    if (!is_bool && !is_quantity) incompatible_op();

    switch (op) {
        case binary_op::add:
        case binary_op::sub:
        case binary_op::lt:
        case binary_op::le:
        case binary_op::gt:
        case binary_op::ge:
        case binary_op::eq:
        case binary_op::ne:
        case binary_op::min:
        case binary_op::max: {
            if (is_bool) incompatible_op();
            if (lhs_q->type != rhs_q->type) incompatible_args();
            type = type_of(lhs);
            break;
        }
        case binary_op::land:
        case binary_op::lor: {
            if (is_bool) {
                type = make_rtype<resolved_boolean>(loc);
                break;
            }
            if (lhs_q->type != rhs_q->type) incompatible_args();
            type = type_of(lhs);
            break;
        }
        case binary_op::dot:
            if (is_bool) incompatible_op();
            type = type_of(rhs);
            break;
        case binary_op::mul:
            if (is_bool) incompatible_op();
            type = make_rtype<resolved_quantity>(lhs_q->type*rhs_q->type, loc);
            break;
        case binary_op::div:
            if (is_bool) incompatible_op();
            type = make_rtype<resolved_quantity>(lhs_q->type/rhs_q->type, loc);
            break;
        case binary_op::pow: {
            if (is_bool) incompatible_op();
            auto rhs_int = std::get_if<resolved_int>(rhs.get());
            if (!rhs_int) {
                throw std::runtime_error(fmt::format("Internal compiler error: operator {} rhs is not a resolved_int "
                                                     "at {}", to_string(op), to_string(loc)));
            }
            type = make_rtype<resolved_quantity>(lhs_q->type^rhs_int->value, loc);
            break;
        }
    }
}

resolved_unary::resolved_unary(unary_op op, r_expr a, const src_location& loc):
    op(op),
    arg(std::move(a)),
    loc(loc)
{
    auto arg_q = std::get_if<resolved_quantity>(type_of(arg).get());
    auto arg_b = std::get_if<resolved_boolean>(type_of(arg).get());

    auto incompatible_op = [&]() {
        throw std::runtime_error(fmt::format("Internal compiler error: cannot apply operand {} to type {} at {}",
                                             to_string(op), to_string(type_of(arg)), to_string(loc)));
    };

    if (!arg_q && !arg_b) incompatible_op();

    bool is_real = (arg_q && arg_q->type.is_real());

    switch (op) {
        case unary_op::exp:
        case unary_op::log:
        case unary_op::cos:
        case unary_op::sin:
        case unary_op::abs:
        case unary_op::exprelr: {
            if (!is_real) incompatible_op();
            type = type_of(arg);
            break;
        }
        case unary_op::lnot:
        case unary_op::neg: {
            type = type_of(arg);
        }
    }
}

std::string to_string(const resolved_mechanism& e, bool include_type, bool expand_var, int indent) {
    auto indent_str = std::string(indent*2, ' ');
    std::string str = indent_str + "(module_expr " + e.name + " " + to_string(e.kind) + "\n";
    for (const auto& p: e.parameters) {
        str += to_string(p, include_type, expand_var, indent+1) + "\n";
    }
    for (const auto& p: e.constants) {
        str += to_string(p, include_type, expand_var, indent+1) + "\n";
    }
    for (const auto& p: e.states) {
        str += to_string(p, include_type, expand_var, indent+1) + "\n";
    }
    for (const auto& p: e.bindings) {
        str += to_string(p, include_type, expand_var, indent+1) + "\n";
    }
    for (const auto& p: e.functions) {
        str += to_string(p, include_type, expand_var, indent+1) + "\n";
    }
    for (const auto& p: e.initializations) {
        str += to_string(p, include_type, expand_var, indent+1) + "\n";
    }
    for (const auto& p: e.on_events) {
        str += to_string(p, include_type, expand_var, indent+1) + "\n";
    }
    for (const auto& p: e.evolutions) {
        str += to_string(p, include_type, expand_var, indent+1) + "\n";
    }
    for (const auto& p: e.effects) {
        str += to_string(p, include_type, expand_var, indent+1) + "\n";
    }
    for (const auto& p: e.exports) {
        str += to_string(p, include_type, expand_var, indent+1) + "\n";
    }
    return str + to_string(e.loc) + ")";
}

// resolved_parameter
std::string to_string(const resolved_parameter& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_parameter\n";
    str += double_indent + e.name + "\n";
    str += to_string(e.value, false, expand_var, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_constant
std::string to_string(const resolved_constant& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_constant\n";
    str += double_indent + e.name + "\n";
    str += to_string(e.value, false, expand_var, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1) + "\n";
    return str + ")";
}

// resolved_state
std::string to_string(const resolved_state& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_state\n";
    str += double_indent + e.name;
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_record_alias
std::string to_string(const resolved_record_alias& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_record_alias\n";
    str += double_indent + e.name;
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_function
std::string to_string(const resolved_function& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_function\n";
    str += double_indent + e.name +  "\n";
    str += to_string(e.type, indent+1) + "\n";

    str += double_indent + "(\n";
    for (const auto& f: e.args) {
        str += to_string(f, true, expand_var, indent+2) + "\n";
    }
    str += double_indent + ")";
    if (include_type) str += "\n" + to_string(e.body, false, expand_var, indent+1);
    return str + ")";
}

// resolved_bind
std::string to_string(const resolved_bind& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_bind\n";
    str += (double_indent + to_string(e.bind));
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    str += double_indent + e.name;
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_initial
std::string to_string(const resolved_initial& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_initial\n";
    str += to_string(e.identifier, false, expand_var, indent+1) + "\n";
    str += to_string(e.value, false, expand_var, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_initial
std::string to_string(const resolved_on_event& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_on_event\n";
    str += to_string(e.argument, false, expand_var, indent+1) + "\n";
    str += to_string(e.identifier, false, expand_var, indent+1) + "\n";
    str += to_string(e.value, false, expand_var, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_evolve
std::string to_string(const resolved_evolve& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_evolve\n";
    str += to_string(e.identifier, false, expand_var, indent+1) + "\n";
    str += to_string(e.value, false, expand_var, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_effect
std::string to_string(const resolved_effect& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_effect\n";
    str += (double_indent + to_string(e.effect));
    if (e.ion) {
        str += ("[" + e.ion.value() + "]");
    }
    str += "\n";
    str += to_string(e.value, false, expand_var, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_export
std::string to_string(const resolved_export& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_export\n";
    str += to_string(e.identifier, false, expand_var, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_call
std::string to_string(const resolved_call& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_call\n";
    str += double_indent + e.f_identifier;
    for (const auto& f: e.call_args) {
        str += "\n" + to_string(f, false, expand_var, indent+1);
    }
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_object
std::string to_string(const resolved_object& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";
    auto triple_indent = double_indent + "  ";

    std::string str = single_indent + "(resolved_object";
    for (unsigned i = 0; i < e.record_fields.size(); ++i) {
        str += "\n" + double_indent + "(\n";
        str += to_string(e.record_fields[i], false, expand_var, indent+2) + "\n";
        str += to_string(e.field_values()[i], false, expand_var, indent+2) + "\n";
        str += double_indent + ")";
    }
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_let
std::string to_string(const resolved_let& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_let\n";
    str += to_string(e.identifier, false, expand_var, indent+1) + "\n";
    str += to_string(e.id_value(), true, expand_var, indent+1) + "\n";
    str += to_string(e.body, true, expand_var, indent+1);
    auto type = to_string(e.type, indent+1);
    if (include_type) str += "\n" + type;
    return str + ")";
}

// resolved_conditional
std::string to_string(const resolved_conditional& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_conditional\n";
    str += to_string(e.condition, false, expand_var, indent+1) + "\n";
    str += to_string(e.value_true, false, expand_var, indent+1) + "\n";
    str += to_string(e.value_false, false, expand_var, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_float
std::string to_string(const resolved_float& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_float\n";

    // get accurate double
    std::ostringstream os;
    os << std::setprecision(std::numeric_limits<double>::max_digits10) << e.value;

    str += double_indent + os.str();
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_int
std::string to_string(const resolved_int& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_int\n";
    str += double_indent + std::to_string(e.value);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_unary
std::string to_string(const resolved_unary& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_unary " + to_string(e.op) + "\n";
    str += to_string(e.arg, false, expand_var, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

// resolved_binary
std::string to_string(const resolved_binary& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_binary " + to_string(e.op) + "\n";
    str += to_string(e.lhs, false, expand_var, indent+1) + "\n";
    str += to_string(e.rhs, false, expand_var, indent+1);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

std::string to_string(const resolved_argument& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_argument \n";
    str += double_indent + e.name;
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

std::string to_string(const resolved_variable& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_variable \n";
    str += double_indent + e.name;
    if (expand_var)   str += "\n" + to_string(e.value, include_type, expand_var, indent+2);
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

std::string to_string(const resolved_field_access& e, bool include_type, bool expand_var, int indent) {
    auto single_indent = std::string(indent*2, ' ');
    auto double_indent = single_indent + "  ";

    std::string str = single_indent + "(resolved_field_access \n";
    str += to_string(e.object, include_type, expand_var, indent+1);
    str += "\n" + double_indent + e.field;
    if (include_type) str += "\n" + to_string(e.type, indent+1);
    return str + ")";
}

std::string to_string(const r_expr & e, bool include_type, bool expand_var, int indent) {
    return std::visit([&](auto&& c){return to_string(c, include_type, expand_var, indent);}, *e);
}

// equality comparison
bool operator!=(const resolved_expr& lhs, const resolved_expr& rhs) {return !(lhs == rhs);}

bool operator==(const resolved_argument& lhs, const resolved_argument& rhs) {
    return (lhs.name == rhs.name) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_variable& lhs, const resolved_variable& rhs) {
    return (lhs.name == rhs.name) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_field_access& lhs, const resolved_field_access& rhs) {
    return (lhs.field == rhs.field) & (*lhs.object == *rhs.object) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_parameter& lhs, const resolved_parameter& rhs) {
    return (lhs.name == rhs.name) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_constant& lhs, const resolved_constant& rhs) {
    return (lhs.name == rhs.name) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_state& lhs, const resolved_state& rhs) {
    return (lhs.name == rhs.name) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_record_alias& lhs, const resolved_record_alias& rhs) {
    return (lhs.name == rhs.name) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_function& lhs, const resolved_function& rhs) {
    if (lhs.args.size() != rhs.args.size()) return false;
    for (unsigned i = 0; i < lhs.args.size(); ++i) {
        if (*lhs.args[i] != *rhs.args[i]) return false;
    }
    return (lhs.name == rhs.name) && (*lhs.body == *rhs.body) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_bind& lhs, const resolved_bind& rhs) {
    return (lhs.bind == rhs.bind) && (lhs.ion == rhs.ion) && (lhs.name == rhs.name) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_initial& lhs, const resolved_initial& rhs) {
    return (*lhs.identifier == *rhs.identifier) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_on_event& lhs, const resolved_on_event& rhs) {
    return (*lhs.argument == *rhs.argument) && (*lhs.identifier == *rhs.identifier) &&
           (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_evolve& lhs, const resolved_evolve& rhs) {
    return (*lhs.identifier == *rhs.identifier) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_effect& lhs, const resolved_effect& rhs) {
    return (lhs.effect == rhs.effect) && (lhs.ion == rhs.ion) && (*lhs.value == *rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_export& lhs, const resolved_export& rhs) {
    return (*lhs.identifier == *rhs.identifier) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_call& lhs, const resolved_call& rhs) {
    if (lhs.call_args.size() != rhs.call_args.size()) return false;
    for (unsigned i = 0; i < lhs.call_args.size(); ++i) {
        if (*lhs.call_args[i] != *rhs.call_args[i]) return false;
    }
    return  (lhs.f_identifier == rhs.f_identifier) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_object& lhs, const resolved_object& rhs) {
    auto lhs_fields = lhs.record_fields;
    auto rhs_fields = rhs.record_fields;
    std::sort(lhs_fields.begin(), lhs_fields.end());
    std::sort(rhs_fields.begin(), rhs_fields.end());

    if (lhs_fields.size() != rhs_fields.size()) return false;
    for (unsigned i = 0; i < lhs_fields.size(); ++i) {
        if (*lhs_fields[i] != *rhs_fields[i]) return false;
    }
    return *lhs.type == *rhs.type;
}

bool operator==(const resolved_let& lhs, const resolved_let& rhs) {
    return (*lhs.identifier == *rhs.identifier) && (*lhs.body == *rhs.body) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_conditional& lhs, const resolved_conditional& rhs) {
    return (*lhs.condition == *rhs.condition) && (*lhs.value_true == *rhs.value_true) &&
           (*lhs.value_false == *rhs.value_false) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_float& lhs, const resolved_float& rhs) {
    return (lhs.value == rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_int& lhs, const resolved_int& rhs) {
    return (lhs.value == rhs.value) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_unary& lhs, const resolved_unary& rhs) {
    return (lhs.op == rhs.op) && (*lhs.arg == *rhs.arg) && (*lhs.type == *rhs.type);
}

bool operator==(const resolved_binary& lhs, const resolved_binary& rhs) {
    return (lhs.op == rhs.op) && (*lhs.lhs == *rhs.lhs) && (*lhs.rhs == *rhs.rhs) && (*lhs.type == *rhs.type);
}

// common member getters
r_type type_of(const r_expr& e) {
    return std::visit([](auto&& c){return c.type;}, *e);
}

src_location location_of(const r_expr& e) {
    return std::visit([](auto&& c){return c.loc;}, *e);
}

} // namespace al
} // namespace resolved_ir
