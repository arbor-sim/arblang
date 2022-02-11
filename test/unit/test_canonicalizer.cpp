#include <cmath>
#include <string>
#include <variant>
#include <vector>

#include <arblang/token.hpp>
#include <arblang/parser.hpp>
#include <arblang/unit_normalizer.hpp>
#include <arblang/canonicalize.hpp>
#include <arblang/resolved_expressions.hpp>
#include <arblang/resolved_types.hpp>

#include "../gtest.h"

using namespace al;
using namespace resolved_ir;
using namespace t_resolved_ir;

// TODO test exceptions properly

TEST(canonicalizer, call) {
    in_scope_map scope_map;
    auto loc = src_location{};
    auto real_type    = make_rtype<resolved_quantity>(normalized_type(quantity::real), loc);
    auto current_type = make_rtype<resolved_quantity>(normalized_type(quantity::current), loc);
    auto voltage_type = make_rtype<resolved_quantity>(normalized_type(quantity::voltage), loc);
    auto bar_type     = make_rtype<resolved_record>(std::vector<std::pair<std::string, r_type>>{{"X", real_type}}, loc);
    auto real_body    = make_rexpr<resolved_float>(0, real_type, loc);

    scope_map.local_map.insert({"bar", resolved_argument("bar", bar_type, loc)});
    scope_map.local_map.insert({"a", resolved_argument("a", real_type, loc)});
    {
        scope_map.func_map.insert({"foo", resolved_function("foo", {}, real_body, real_type, loc)});

        std::string expr = "foo()";
        auto p = parser(expr);
        auto call = p.parse_call();
        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        std::cout << to_string(call_canon) << std::endl;
        std::cout << std::endl;
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", real_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        scope_map.func_map.insert({"foo2", resolved_function("foo2", {arg0, arg1}, real_body, real_type, loc)});

        std::string expr = "foo2(2, 1)";
        auto p = parser(expr);
        auto call = p.parse_call();
        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        std::cout << to_string(call_canon) << std::endl;
        std::cout << std::endl;
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", real_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        auto arg2 = make_rexpr<resolved_argument>("c", current_type, loc);
        scope_map.func_map.insert({"foo_bar", resolved_function("foo_bar", {arg0, arg1, arg2}, real_body, real_type, loc)});

        std::string expr = "foo_bar(2.5, a, -1 [A])";
        auto p = parser(expr);
        auto call = p.parse_call();
        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        std::cout << to_string(call_canon) << std::endl;
        std::cout << std::endl;
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", real_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        scope_map.func_map.insert({"bar", resolved_function("bar", {arg0, arg1}, real_body, real_type, loc)});

        std::string expr = "bar(1+4, foo())";
        auto p = parser(expr);
        auto call = p.parse_call();
        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        std::cout << to_string(call_canon) << std::endl;
        std::cout << std::endl;
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", voltage_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        scope_map.func_map.insert({"baz", resolved_function("baz", {arg0, arg1}, real_body, real_type, loc)});

        std::string expr = "baz(let b: voltage = 6 [mV]; b, bar.X)";
        auto p = parser(expr);
        auto call = p.parse_call();
        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        std::cout << to_string(call_canon) << std::endl;
        std::cout << std::endl;
    }
}

TEST(canonicalizer, let) {
    auto loc = src_location{};
    auto real_type    = make_rtype<resolved_quantity>(normalized_type(quantity::real), loc);
    auto current_type = make_rtype<resolved_quantity>(normalized_type(quantity::current), loc);
    auto voltage_type = make_rtype<resolved_quantity>(normalized_type(quantity::voltage), loc);
    auto conductance_type = make_rtype<resolved_quantity>(normalized_type(quantity::conductance), loc);
    auto real_body    = make_rexpr<resolved_float>(0, real_type, loc);

    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"a", resolved_argument("a", real_type, loc)});

        std::string expr = "let a = a + 5; let a = a + 5; a;";
        auto p = parser(expr);
        auto let = p.parse_let();
        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);
        std::cout << to_string(let_canon) << std::endl;
        std::cout << std::endl;
        std::cout << to_string(let_ssa) << std::endl;
        std::cout << std::endl;
    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"a", resolved_argument("a", voltage_type, loc)});
        scope_map.local_map.insert({"s", resolved_argument("s", conductance_type, loc)});

        std::string expr = "let b:voltage = a + a*5; let c:current = b*s; c*a*b)";
        auto p = parser(expr);
        auto let = p.parse_let();
        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        std::cout << to_string(let_canon) << std::endl;
        std::cout << std::endl;
    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"a",  resolved_argument("a", voltage_type, loc)});
        scope_map.local_map.insert({"s",  resolved_argument("s", conductance_type, loc)});
        scope_map.func_map.insert({"foo", resolved_function("foo",
                                                            {make_rexpr<resolved_argument>("a", current_type, loc)},
                                                            real_body, real_type, loc)});

        std::string expr = "let b = let x = a+5 [mV] /2; x*s; let c = foo(b)*foo(a*s); c/2.1 [A];";
        auto p = parser(expr);
        auto let = p.parse_let();
        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        std::cout << to_string(let_canon) << std::endl;
        std::cout << std::endl;
    }
}

TEST(canonicalizer, with) {
    auto loc = src_location{};
    auto real_type    = make_rtype<resolved_quantity>(normalized_type(quantity::real), loc);
    auto current_type = make_rtype<resolved_quantity>(normalized_type(quantity::current), loc);
    auto voltage_type = make_rtype<resolved_quantity>(normalized_type(quantity::voltage), loc);
    auto conductance_type = make_rtype<resolved_quantity>(normalized_type(quantity::conductance), loc);
    auto real_body    = make_rexpr<resolved_float>(0, real_type, loc);

    std::vector<std::pair<std::string, r_type>> foo_fields = {{"a", voltage_type}, {"b", current_type}};
    auto foo_type = make_rtype<resolved_record>(foo_fields, loc);
    std::vector<std::pair<std::string, r_type>> bar_fields = {{"X", real_type}, {"Y", foo_type}};
    auto bar_type = make_rtype<resolved_record>(bar_fields, loc);

    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"t", resolved_argument("t", real_type, loc)});
        scope_map.local_map.insert({"q", resolved_argument("q", real_type, loc)});
        scope_map.func_map.insert({"foo", resolved_function("foo", {}, real_body, real_type, loc)});
        scope_map.type_map.insert({"foo", foo_type});
        scope_map.type_map.insert({"bar", bar_type});

        std::string expr = "let B:bar = {X = t + (5 - q); Y = {a = 2[V]; b = foo()*1[A];};};\n"
                           "with B.Y;\n"
                           "let r = a/b;\n"
                           "with B;\n"
                           "X*r/3;\n";

        auto p = parser(expr);
        auto let = p.parse_let();
        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);
        std::cout << to_string(let_canon) << std::endl;
        std::cout << std::endl;
        std::cout << to_string(let_ssa) << std::endl;
        std::cout << std::endl;
    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"t", resolved_argument("t", real_type, loc)});
        scope_map.local_map.insert({"q", resolved_argument("q", real_type, loc)});
        scope_map.func_map.insert({"foo", resolved_function("foo", {}, real_body, real_type, loc)});
        scope_map.type_map.insert({"foo", foo_type});
        scope_map.type_map.insert({"bar", bar_type});

        std::string expr = "let B:bar = {X = t + (5 - q); Y = {a = 2[V]; b = foo()*1[A];};};\n"
                           "with B.Y;\n"
                           "let r = a/b;\n"
                           "let B:foo = {a = 3[V]; b=-2[A];};\n"
                           "with B;\n"
                           "X*r/3;\n";

        auto p = parser(expr);
        auto let = p.parse_let();
        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);
        std::cout << to_string(let_canon) << std::endl;
        std::cout << std::endl;
        std::cout << to_string(let_ssa) << std::endl;
        std::cout << std::endl;
    }
    {
        in_scope_map scope_map;
        scope_map.type_map.insert({"foo", foo_type});

        std::string expr = "let A:foo = {a = 2[V]; b = 1[A];};\n"
                           "with A;\n"
                           "let a = a/b;\n"
                           "with A;\n"
                           "a;\n";

        auto p = parser(expr);
        auto let = p.parse_let();
        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);
        std::cout << to_string(let_canon) << std::endl;
        std::cout << std::endl;
        std::cout << to_string(let_ssa) << std::endl;
        std::cout << std::endl;
    }
}

TEST(canonicalizer, conditional) {
    auto loc = src_location{};
    auto real_type    = make_rtype<resolved_quantity>(normalized_type(quantity::real), loc);
    auto current_type = make_rtype<resolved_quantity>(normalized_type(quantity::current), loc);
    auto voltage_type = make_rtype<resolved_quantity>(normalized_type(quantity::voltage), loc);
    auto conductance_type = make_rtype<resolved_quantity>(normalized_type(quantity::conductance), loc);
    auto real_body    = make_rexpr<resolved_float>(0, real_type, loc);

    std::vector<std::pair<std::string, r_type>> foo_fields = {{"a", voltage_type}, {"b", current_type}};
    auto foo_type = make_rtype<resolved_record>(foo_fields, loc);
    std::vector<std::pair<std::string, r_type>> bar_fields = {{"X", real_type}, {"Y", foo_type}};
    auto bar_type = make_rtype<resolved_record>(bar_fields, loc);

    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"t", resolved_argument("t", real_type, loc)});

        std::string expr = "if t == 4 then let a=3; a*4 else 15.5;";

        auto p = parser(expr);
        auto ifstmt = p.parse_conditional();
        auto if_normal = normalize(ifstmt);
        auto if_resolved = resolve(if_normal, scope_map);
        auto if_canon = canonicalize(if_resolved);
        auto if_ssa = single_assign(if_canon);
        std::cout << to_string(if_canon) << std::endl;
        std::cout << std::endl;
        std::cout << to_string(if_ssa) << std::endl;
        std::cout << std::endl;
    }
    {
        in_scope_map scope_map;
        scope_map.func_map.insert({"foo", resolved_function("foo", {}, real_body, real_type, loc)});
        scope_map.type_map.insert({"foo", foo_type});
        scope_map.type_map.insert({"bar", bar_type});
        scope_map.local_map.insert({"t", resolved_argument("t", real_type, loc)});
        scope_map.local_map.insert({"a", resolved_argument("a", real_type, loc)});
        scope_map.local_map.insert({"obar", resolved_argument("obar", bar_type, loc)});

        std::string expr = "if (if t == 4 then a>3 else a<4)\n"
                           "then (if obar.X == 5 then obar.Y else {a=3[V]; b=5[mA];})\n"
                           "else {a=foo()*3[V]; b=7000[mA];});";

        auto p = parser(expr);
        auto ifstmt = p.parse_conditional();
        auto if_normal = normalize(ifstmt);
        auto if_resolved = resolve(if_normal, scope_map);
        auto if_canon = canonicalize(if_resolved);
        auto if_ssa = single_assign(if_canon);
        std::cout << to_string(if_canon) << std::endl;
        std::cout << std::endl;
        std::cout << to_string(if_ssa) << std::endl;
        std::cout << std::endl;
    }
}

TEST(canonicalizer, object) {
}