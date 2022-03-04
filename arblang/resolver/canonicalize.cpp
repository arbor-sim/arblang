#include <string>
#include <unordered_set>

#include <fmt/core.h>

#include <arblang/resolver/canonicalize.hpp>
#include <arblang/util/unique_name.hpp>

#include "../util/rexp_helpers.hpp"

namespace al {
namespace resolved_ir {

// Canonicalize
resolved_mechanism canonicalize(const resolved_mechanism& e) {
    std::unordered_set<std::string> reserved;
    std::unordered_map<std::string, r_expr> rewrites;
    std::string pref = "t";
    resolved_mechanism mech;
    for (const auto& c: e.constants) {
        reserved.clear();
        rewrites.clear();
        mech.constants.push_back(canonicalize(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.parameters) {
        reserved.clear();
        rewrites.clear();
        mech.parameters.push_back(canonicalize(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.bindings) {
        reserved.clear();
        rewrites.clear();
        mech.bindings.push_back(canonicalize(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.states) {
        reserved.clear();
        rewrites.clear();
        mech.states.push_back(canonicalize(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.functions) {
        reserved.clear();
        rewrites.clear();
        mech.functions.push_back(canonicalize(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.initializations) {
        reserved.clear();
        rewrites.clear();
        mech.initializations.push_back(canonicalize(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.evolutions) {
        reserved.clear();
        rewrites.clear();
        mech.evolutions.push_back(canonicalize(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.effects) {
        reserved.clear();
        rewrites.clear();
        mech.effects.push_back(canonicalize(c, reserved, rewrites, pref));
    }
    for (const auto& c: e.exports) {
        reserved.clear();
        rewrites.clear();
        mech.exports.push_back(canonicalize(c, reserved, rewrites, pref));
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return mech;
}

r_expr canonicalize(const resolved_record_alias& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation.");
}

r_expr canonicalize(const resolved_argument& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    return make_rexpr<resolved_argument>(e);
}

r_expr canonicalize(const resolved_variable& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    if (rewrites.count(e.name)) {
        return rewrites.at(e.name);
    }
    return make_rexpr<resolved_variable>(e);
}

r_expr canonicalize(const resolved_parameter& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    auto val_canon = canonicalize(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_parameter>(e.name, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_constant& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    auto val_canon = canonicalize(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_constant>(e.name, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_state& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    return make_rexpr<resolved_state>(e);
}

r_expr canonicalize(const resolved_function& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    auto body_canon = canonicalize(e.body, reserved, rewrites, pref);
    return make_rexpr<resolved_function>(e.name, e.args, body_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_bind& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    return make_rexpr<resolved_bind>(e);
}

r_expr canonicalize(const resolved_initial& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    auto val_canon = canonicalize(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_initial>(e.identifier, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_evolve& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    auto val_canon = canonicalize(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_evolve>(e.identifier, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_effect& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    auto val_canon = canonicalize(e.value, reserved, rewrites, pref);
    return make_rexpr<resolved_effect>(e.effect, e.ion, val_canon, e.type, e.loc);
}

r_expr canonicalize(const resolved_export& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    return make_rexpr<resolved_export>(e);
}

r_expr canonicalize(const resolved_call& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    // Canonicalize the arguments of the function call.
    // If an argument is a let statement:
    //   1. The let statement becomes the new body of a preceding
    //      let statement if it exists.
    //   2. The innermost body of the let statement become the new
    //      argument of the call
    // Example:
    //   foo(a+b);
    // 1. 'a+b' is canonicalized into 'let _t0 = a+b; _t0;'
    // 2. foo(a+b) becomes 'let _t0 = a+b; let t1_ = foo(_t0); _t1;'
    //
    //   foo(a+b, c*d) becomes 'let _t0 = a+b; let t1_ = c*d; let _t2 = foo(_t0, _t1); _t2;'

    std::vector<r_expr> args_canon;

    bool has_let = false;
    resolved_let let_outer;
    for (const auto& arg: e.call_args) {
        auto arg_canon = canonicalize(arg, reserved, rewrites, pref);
        if (auto let_opt = get_let(arg_canon)) {
            auto let_arg = let_opt.value();

            // The innermost body of let_arg is the new call argument
            args_canon.push_back(get_innermost_body(&let_arg));

            // If let_arg is the first let statement encountered
            // it is assigned as the 'outer' let_statement
            // otherwise it becomes the innermost body of the
            // outer let statement
            if (!has_let) {
                let_outer = let_arg;
            } else {
                set_innermost_body(&let_outer, arg_canon);
            }
            has_let = true;
        }
        else {
            args_canon.push_back(arg_canon);
        }
    }

    // The new call_expression can be formed from the new arguments
    auto call_canon = make_rexpr<resolved_call>(e.f_identifier, args_canon, e.type, e.loc);

    // A new let_expression is formed: `let _tx = call(args); _tx;`

    // N.B.
    // The identifier _tx holds a pointer to the value (call(args)).
    // This is done so that any other expressions using _tx can refer
    // to its value if needed.

    auto temp_expr = make_rexpr<resolved_variable>(unique_local_name(reserved, pref), call_canon, e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    // Return the full let expression
    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_object& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    // Canonicalize the fields of an object.
    // If the value of a field is a let statement:
    //   1. The let statement becomes the new body of a preceding
    //      let statement if it exists.
    //   2. The innermost body of the let statement become the new
    //      value of the field
    // Example:
    //   {m = foo(); n = a+b;};
    // 1. 'foo()' is canonicalized into 'let _t0 = foo(); _t0;'
    // 1. 'a+b' is canonicalized into 'let _t1 = a+b; _t1;'
    // 2. {m = foo(); n = a+b;} becomes 'let _t0 = foo(); let _t1 = a+b; let _t2 = {m = _t0; n = _t1;}; _t2;'

    std::vector<r_expr> values_canon;

    bool has_let = false;
    resolved_let let_outer;
    for (const auto& arg: e.field_values()) {
        auto arg_canon = canonicalize(arg, reserved, rewrites, pref);
        if (auto let_opt = get_let(arg_canon)) {
            auto let_val = let_opt.value();

            // The innermost body of let_val is the new call argument
            values_canon.push_back(get_innermost_body(&let_val));

            // If let_val is the first let statement encountered
            // it is assigned as the 'outer' let_statement
            // otherwise it becomes the innermost body of the
            // outer let statement
            if (!has_let) {
                let_outer = let_val;
            }
            else {
                set_innermost_body(&let_outer, arg_canon);
            }
            has_let = true;
        }
        else {
            values_canon.push_back(arg_canon);
        }
    }
    // A new let_expression is formed: `let _tx = {fields = values}; _tx;`

    // N.B.
    // The identifier _tx holds a pointer to the value (call(args)).
    // This is done so that any other expressions using _tx can refer
    // to its value if needed.
    auto object_canon = make_rexpr<resolved_object>(e.field_names(), values_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_variable>(unique_local_name(reserved, pref), object_canon, e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_let& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    // Canonicalize the value and body of a let statement.
    // If the canonicalized value is a let statement:
    //   1. The innermost body of the canonicalized value becomes
    //      the new value of the original let statement.
    //   2. The original let statement becomes the new body of
    //      the canonicalized value.
    // Example:
    //   let x = a + b + c; x;
    // 1. 'a + b + c' is canonicalized into 'let _t0 = a+b; let _t1 = _t0+c; _t1;'
    // 2. To canonicalize 'let x = (let _t0 = a+b; let _t1 = _t0+c; _t1;); x;' perform the
    //    procedure outlined above to get 'let _t0 = a+b; let _t1 = _t0+c; let x = _t1; x;'

    auto val_canon = canonicalize(e.id_value(), reserved, rewrites, pref);

    auto var_name = e.id_name();
    auto var_canon = make_rexpr<resolved_variable>(e.id_name(), val_canon, type_of(val_canon), location_of(val_canon));
    rewrites.insert({var_name, var_canon});

    auto body_canon = canonicalize(e.body, reserved, rewrites, pref);

    auto let_outer = resolved_let(var_canon, body_canon, e.type, e.loc);

    if (auto let_opt = get_let(val_canon)) {
        auto let_val = let_opt.value();

        let_outer.id_value(get_innermost_body(&let_val));
        set_innermost_body(&let_val, make_rexpr<resolved_let>(let_outer));

        return make_rexpr<resolved_let>(let_val);
    }

    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_conditional& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    // Canonicalize the condition, true value and false value of a conditional statement.
    // If any of the canonicalized values is a let statement:
    //   1. The let statement becomes the new body of a preceding
    //      let statement if it exists.
    //   2. The innermost body of the let statement become the new
    //      value;
    // Example:
    //   if a < b; then x/y; else y/x;
    // 1. 'a<b' is canonicalized into 'let _t0 = a<b; _t0;'
    // 2. 'x/y' is canonicalized into 'let _t1 = x/y; _t1;'
    // 3. 'y/x' is canonicalized into 'let _t2 = y/x; _t2;'
    // 4. 'if a < b; then x/y; else y/x;' is canonicalized into
    //    'let _t0 = a<b; let _t1 = x/y; let _t2 = y/x; let t3_ = (if _t0; then _t1; else _t2;); _t3;'

    auto cond_canon = canonicalize(e.condition, reserved, rewrites, pref);
    auto true_canon = canonicalize(e.value_true, reserved, rewrites, pref);
    auto false_canon = canonicalize(e.value_false, reserved, rewrites, pref);

    resolved_let let_outer;
    bool has_let = false;
    if (auto let_opt = get_let(cond_canon)) {
        auto let_cond = let_opt.value();
        let_outer = let_cond;
        cond_canon = get_innermost_body(&let_cond);
        has_let = true;
    }
    if (auto let_opt = get_let(true_canon)) {
        auto let_true = let_opt.value();
        if (!has_let) {
            let_outer = let_true;
        }
        else {
            set_innermost_body(&let_outer, true_canon);
        }
        has_let = true;
        true_canon = get_innermost_body(&let_true);
    }
    if (auto let_opt = get_let(false_canon)) {
        auto let_false = let_opt.value();
        if (!has_let) {
            let_outer = let_false;
        }
        else {
            set_innermost_body(&let_outer, false_canon);
        }
        has_let = true;
        false_canon = get_innermost_body(&let_false);
    }

    auto if_canon = make_rexpr<resolved_conditional>(cond_canon, true_canon, false_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_variable>(unique_local_name(reserved, pref), if_canon, e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_float& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    return make_rexpr<resolved_float>(e);
}

r_expr canonicalize(const resolved_int& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    return make_rexpr<resolved_int>(e);
}

r_expr canonicalize(const resolved_unary& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    // Canonicalize the argument of a unary expression.
    // If the canonicalized argument is a let statement:
    //   1. The let statement is saved and later used to wrap a
    //      new let statement containing the value of the unary
    //      expression
    //   2. The innermost body of the let statement become the new
    //      argument.
    // Example:
    //   exp(a+b);
    // 1. 'a+b' is canonicalized into 'let _t0 = a+b; _t0;'
    // 2. 'exp(a+b)' is canonicalized into 'let _t0 = a+b; let _t1 = exp(_t0); _t1;'

    auto arg_canon = canonicalize(e.arg, reserved, rewrites, pref);

    resolved_let let_outer;
    bool has_let = false;
    if (auto let_opt = get_let(arg_canon)) {
        auto let_first = let_opt.value();
        arg_canon = get_innermost_body(&let_first);
        let_outer = let_first;
        has_let = true;
    }
    auto unary_canon = make_rexpr<resolved_unary>(e.op, arg_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_variable>(unique_local_name(reserved, pref), unary_canon, e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_binary& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    // Canonicalize the lhs and rhs of a binary expression.
    // If the canonicalized lhs is a let statement:
    //   1. The let statement is saved and later used to wrap a
    //      new let statement containing the value of the binary
    //      expression
    //   2. The innermost body of the let statement become the new
    //      lhs.
    // If the canonicalized rhs is a let statement:
    //   1. The let statement is becomes the body of the saved lhs
    //      let statement if it exists. Otherwise, it is saved and
    //      later used to wrap a new let statement containing the
    //      value of the binary expression
    //   2. The innermost body of the let statement become the new
    //      rhs.
    // Example:
    //   foo() + exp(a);
    // 1. 'foo()' is canonicalized into 'let _t0 = foo(); _t0;'
    // 2. 'exp(a)' is canonicalized into 'let _t1 = exp(a); _t1;'
    // 3. 'foo() + exp(a)' is canonicalized into
    //    'let _t0=foo(); let _t1=exp(a); let _t2=_t0+_t1; _t2'

    auto lhs_canon = canonicalize(e.lhs, reserved, rewrites, pref);
    auto rhs_canon = canonicalize(e.rhs, reserved, rewrites, pref);

    resolved_let let_outer;
    bool has_let = false;
    if (auto let_opt = get_let(lhs_canon)) {
        auto let_first = let_opt.value();
        let_outer = let_first;
        lhs_canon = get_innermost_body(&let_first);
        has_let = true;
    }
    if (auto let_opt = get_let(rhs_canon)) {
        auto let_first = let_opt.value();
        if (!has_let) {
            let_outer = let_first;
        }
        else {
            set_innermost_body(&let_outer, rhs_canon);
        }
        rhs_canon = get_innermost_body(&let_first);
        has_let = true;
    }

    auto binary_canon = make_rexpr<resolved_binary>(e.op, lhs_canon, rhs_canon, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_variable>(unique_local_name(reserved, pref), binary_canon, e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const resolved_field_access& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    // Canonicalize the object of a field access expression.
    // If the canonicalized object is a let statement:
    //   1. The let statement is saved and later used to wrap a
    //      new let statement containing the value of the unary
    //      expression
    //   2. The innermost body of the let statement become the new
    //      argument.
    // Example:
    //   foo(a).m;
    // 1. 'foo(a)' is canonicalized into 'let _t0 = foo(a); _t0;'
    // 2. 'foo(a).m' is canonicalized into 'let _t0 = foo(a); let _t1 = _t0.m; _t1;'

    auto obj_canon = canonicalize(e.object, reserved, rewrites, pref);

    resolved_let let_outer;
    bool has_let = false;
    if (auto let_opt = get_let(obj_canon)) {
        auto let_obj = let_opt.value();
        let_outer = let_obj;
        obj_canon = get_innermost_body(&let_obj);
        has_let = true;
    }

    auto access_canon = make_rexpr<resolved_field_access>(obj_canon, e.field, e.type, e.loc);
    auto temp_expr = make_rexpr<resolved_variable>(unique_local_name(reserved, pref), access_canon, e.type, e.loc);
    auto let_wrapper = make_rexpr<resolved_let>(temp_expr, temp_expr, e.type, e.loc);

    if (!has_let) return let_wrapper;

    set_innermost_body(&let_outer, let_wrapper);
    return make_rexpr<resolved_let>(let_outer);
}

r_expr canonicalize(const r_expr& e,
                    std::unordered_set<std::string>& reserved,
                    std::unordered_map<std::string, r_expr>& rewrites,
                    const std::string& pref)
{
    return std::visit([&](auto& c) {return canonicalize(c, reserved, rewrites, pref);}, *e);
}

r_expr canonicalize(const r_expr& e, const std::string& pref) {
    std::unordered_set<std::string> reserved;
    std::unordered_map<std::string, r_expr> rewrites;
    return canonicalize(e, reserved, rewrites, pref);
}

} // namespace resolved_ir
} // namespace al