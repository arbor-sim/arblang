#include <cmath>
#include <string>
#include <variant>
#include <vector>

#include <arblang/optimizer/optimizer.hpp>
#include <arblang/optimizer/inline_func.hpp>
#include <arblang/parser/token.hpp>
#include <arblang/parser/parser.hpp>
#include <arblang/parser/normalizer.hpp>
#include <arblang/pre_printer/printable_mechanism.hpp>
#include <arblang/printer/print_header.hpp>
#include <arblang/resolver/canonicalize.hpp>
#include <arblang/resolver/resolve.hpp>
#include <arblang/resolver/resolved_expressions.hpp>
#include <arblang/resolver/resolved_types.hpp>
#include <arblang/resolver/single_assign.hpp>
#include <arblang/solver/solve_ode.hpp>
#include <arblang/solver/solve.hpp>
#include <arblang/util/custom_hash.hpp>
#include <arblang/util/pretty_printer.hpp>

#include "../gtest.h"

using namespace al;
using namespace resolved_ir;
using namespace resolved_type_ir;

r_expr get_innermost_body(resolved_let* const let) {
    resolved_let* let_last = let;
    while (auto let_next = std::get_if<resolved_let>(let_last->body.get())) {
        let_last = let_next;
    }
    return let_last->body;
}

TEST(custom_hash, map) {
    auto loc = src_location{};
    auto real_type    = make_rtype<resolved_quantity>(normalized_type(quantity::real), loc);
    auto real_body    = make_rexpr<resolved_float>(0, real_type, loc);

    auto t0 = resolved_argument("t", real_type, loc);
    auto t1 = resolved_argument("t", real_type, loc);
    auto t2 = resolved_argument("a", real_type, loc);
    auto t3 = resolved_binary(binary_op::add, make_rexpr<resolved_argument>(t0), make_rexpr<resolved_argument>(t2), real_type, loc);
    auto t4 = resolved_binary(binary_op::add, make_rexpr<resolved_argument>(t0), make_rexpr<resolved_argument>(t2), real_type, loc);
    auto t5 = resolved_function("foo", {make_rexpr<resolved_argument>(t0)}, real_body, real_type, loc);
    auto t6 = resolved_function("foo", {make_rexpr<resolved_argument>(t0)}, real_body, real_type, loc);
    std::unordered_map<resolved_expr, int> map;

    map.insert({t0, 0});
    map.insert({t1, 1});
    map.insert({t2, 2});
    map.insert({t3, 3});
    map.insert({t4, 4});
    map.insert({t5, 5});
    map.insert({t6, 6});
}

TEST(canonicalizer, call) {
    in_scope_map scope_map;
    auto loc = src_location{};
    auto real_type    = make_rtype<resolved_quantity>(normalized_type(quantity::real), loc);
    auto current_type = make_rtype<resolved_quantity>(normalized_type(quantity::current), loc);
    auto voltage_type = make_rtype<resolved_quantity>(normalized_type(quantity::voltage), loc);
    auto bar_type     = make_rtype<resolved_record>(std::vector<std::pair<std::string, r_type>>{{"X", real_type}}, loc);
    auto real_body    = make_rexpr<resolved_float>(0, real_type, loc);

    scope_map.local_map.insert({"bar", make_rexpr<resolved_argument>("bar", bar_type, loc)});
    scope_map.local_map.insert({"a", make_rexpr<resolved_argument>("a", real_type, loc)});
    {
        scope_map.func_map.insert({"foo", make_rexpr<resolved_function>("foo", std::vector<r_expr>{}, real_body, real_type, loc)});

        std::string p_expr = "foo()";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved, "t");
        auto call_ssa = single_assign(call_canon, "r");

        auto opt = optimizer(call_ssa);
        auto call_opt = opt.optimize();

        std::string expected_opt = "let _t0:real = foo();\n"
                                   "_t0;";

        std::string expanded_ans = "(variable _t0\n"
                                   "  (call foo))";

        EXPECT_EQ(expected_opt, pretty_print(call_opt));

        auto let = std::get_if<resolved_let>(call_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(let)));
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", real_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        scope_map.func_map.insert({"foo2", make_rexpr<resolved_function>("foo2", std::vector<r_expr>{arg0, arg1}, real_body, real_type, loc)});

        std::string p_expr = "foo2(2, 1)";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved, "t");
        auto call_ssa = single_assign(call_canon, "r");

        auto opt = optimizer(call_ssa);
        auto call_opt = opt.optimize();

        std::string expected_opt = "let _t0:real = foo2(2:real, 1:real);\n"
                                   "_t0;";

        std::string expanded_ans = "(variable _t0\n"
                                   "  (call foo2\n"
                                   "    (2)\n"
                                   "    (1)))";

        EXPECT_EQ(expected_opt, pretty_print(call_opt));

        auto var = std::get_if<resolved_let>(call_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", real_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        auto arg2 = make_rexpr<resolved_argument>("c", current_type, loc);
        scope_map.func_map.insert({"foo_bar", make_rexpr<resolved_function>("foo_bar", std::vector<r_expr>{arg0, arg1, arg2}, real_body, real_type, loc)});

        std::string p_expr = "foo_bar(2.5, a, -1 [A])";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved, "t");
        auto call_ssa = single_assign(call_canon, "r");

        auto opt = optimizer(call_ssa);
        auto call_opt = opt.optimize();

        std::string expected_opt = "let _t1:real = foo_bar(2.5:real, a, -1:A^1);\n"
                                   "_t1;";

        std::string expanded_ans = "(variable _t1\n"
                                   "  (call foo_bar\n"
                                   "    (2.5)\n"
                                   "    (argument a)\n"
                                   "    (-1)))";

        EXPECT_EQ(expected_opt, pretty_print(call_opt));

        auto var = std::get_if<resolved_let>(call_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", real_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        scope_map.func_map.insert({"foo", make_rexpr<resolved_function>("foo", std::vector<r_expr>{}, real_body, real_type, loc)});
        scope_map.func_map.insert({"bar", make_rexpr<resolved_function>("bar", std::vector<r_expr>{arg0, arg1}, real_body, real_type, loc)});

        std::string p_expr = "bar(1+4, foo())";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved, "t");
        auto call_ssa = single_assign(call_canon, "r");

        auto opt = optimizer(call_ssa);
        auto call_opt = opt.optimize();

        std::string expected_opt = "let _t1:real = foo();\n"
                                   "let _t2:real = bar(5:real, _t1);\n"
                                   "_t2;";

        std::string expanded_ans = "(variable _t2\n"
                                   "  (call bar\n"
                                   "    (5)\n"
                                   "    (variable _t1\n"
                                   "      (call foo))))";

        EXPECT_EQ(expected_opt, pretty_print(call_opt));

        auto var = std::get_if<resolved_let>(call_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", voltage_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        scope_map.func_map.insert({"baz", make_rexpr<resolved_function>("baz", std::vector<r_expr>{arg0, arg1}, real_body, real_type, loc)});

        std::string p_expr = "baz(let b:voltage = 6 [mV]; b, bar.X)";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved, "t");
        auto call_ssa = single_assign(call_canon, "r");

        auto opt = optimizer(call_ssa);
        auto call_opt = opt.optimize();

        std::string expected_opt = "let _t0:real = bar.X;\n"
                                   "let _t1:real = baz(0.0060000000000000001:m^2*Kg^1*s^-3*A^-1, _t0);\n"
                                   "_t1;";

        std::string expanded_ans = "(variable _t1\n"
                                   "  (call baz\n"
                                   "    (0.0060000000000000001)\n"
                                   "    (variable _t0\n"
                                   "      (access\n"
                                   "        (argument bar)\n"
                                   "        (X)))))";

        EXPECT_EQ(expected_opt, pretty_print(call_opt));

        auto var = std::get_if<resolved_let>(call_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
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

        std::string p_expr = "let a = 1; let a = a + 5; let a = a + 5; a;";
        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "11:real";
        EXPECT_EQ(expected_opt, pretty_print(let_opt));
    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"a", make_rexpr<resolved_argument>("a", voltage_type, loc)});
        scope_map.local_map.insert({"s", make_rexpr<resolved_argument>("s", conductance_type, loc)});

        std::string p_expr = "let b:voltage = a + a*5; let c:current = b*s; c*a*b)";
        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "let _t0:m^2*Kg^1*s^-3*A^-1 = a*5:real;\n"
                                   "let _t1:m^2*Kg^1*s^-3*A^-1 = a+_t0;\n"
                                   "let _t2:A^1 = _t1*s;\n"
                                   "let _t3:m^2*Kg^1*s^-3 = _t2*a;\n"
                                   "let _t4:m^4*Kg^2*s^-6*A^-1 = _t3*_t1;\n"
                                   "_t4;";

        std::string expanded_ans = "(variable _t4\n"
                                   "  (*\n"
                                   "    (variable _t3\n"
                                   "      (*\n"
                                   "        (variable _t2\n"
                                   "          (*\n"
                                   "            (variable _t1\n"
                                   "              (+\n"
                                   "                (argument a)\n"
                                   "                (variable _t0\n"
                                   "                  (*\n"
                                   "                    (argument a)\n"
                                   "                    (5)))))\n"
                                   "            (argument s)))\n"
                                   "        (argument a)))\n"
                                   "    (variable _t1\n"
                                   "      (+\n"
                                   "        (argument a)\n"
                                   "        (variable _t0\n"
                                   "          (*\n"
                                   "            (argument a)\n"
                                   "            (5)))))))";

        EXPECT_EQ(expected_opt, pretty_print(let_opt));

        auto var = std::get_if<resolved_let>(let_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"a",  make_rexpr<resolved_argument>("a", voltage_type, loc)});
        scope_map.local_map.insert({"s",  make_rexpr<resolved_argument>("s", conductance_type, loc)});
        std::vector<r_expr> foo_args = {make_rexpr<resolved_argument>("a", current_type, loc)};
        scope_map.func_map.insert({"foo", make_rexpr<resolved_function>("foo", foo_args, real_body, real_type, loc)});

        std::string p_expr = "let b = let x = a+5 [mV] /2; x*s; let c = foo(b)*foo(a*s); c/2.1 [A];";
        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        // let t1_:voltage = a+0.0025; // x
        // let t2_:A^1 = t1_*s;    // b
        // let t3_:real = foo(t2_);    // foo(b)
        // let t4_:A^1 = a*s;      //
        // let t5_:real = foo(t4_);    // foo(a*s)
        // let t6_:real = t3_*t5_;     // c
        // let t7_:A^-1 = t6_/2.1;  // c/2.1
        // t7_;

        std::string expected_opt = "let _t1:m^2*Kg^1*s^-3*A^-1 = a+0.0025000000000000001:m^2*Kg^1*s^-3*A^-1;\n"
                                   "let _t2:A^1 = _t1*s;\n"
                                   "let _t3:real = foo(_t2);\n"
                                   "let _t4:A^1 = a*s;\n"
                                   "let _t5:real = foo(_t4);\n"
                                   "let _t6:real = _t3*_t5;\n"
                                   "let _t7:A^-1 = _t6*0.47619047619047616:A^-1;\n"
                                   "_t7;";

        std::string expanded_ans = "(variable _t7\n"
                                   "  (*\n"
                                   "    (variable _t6\n"
                                   "      (*\n"
                                   "        (variable _t3\n"
                                   "          (call foo\n"
                                   "            (variable _t2\n"
                                   "              (*\n"
                                   "                (variable _t1\n"
                                   "                  (+\n"
                                   "                    (argument a)\n"
                                   "                    (0.0025000000000000001)))\n"
                                   "                (argument s)))))\n"
                                   "        (variable _t5\n"
                                   "          (call foo\n"
                                   "            (variable _t4\n"
                                   "              (*\n"
                                   "                (argument a)\n"
                                   "                (argument s)))))))\n"
                                   "    (0.47619047619047616)))";

        EXPECT_EQ(expected_opt, pretty_print(let_opt));

        auto var = std::get_if<resolved_let>(let_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
}

TEST(canonicalizer, object) {
    auto loc = src_location{};
    auto real_type    = make_rtype<resolved_quantity>(normalized_type(quantity::real), loc);
    auto real_body    = make_rexpr<resolved_float>(0, real_type, loc);
    {
        in_scope_map scope_map;
        scope_map.func_map.insert(
                {"foo", make_rexpr<resolved_function>("foo", std::vector<r_expr>{}, real_body, real_type, loc)});

        std::string p_expr = "let q = {a=foo()*3[V]; b=7000[mA];}; "
                             "q.a/q.b";

        auto p = parser(p_expr);
        auto obj = p.parse_let();

        auto obj_normal = normalize(obj);
        auto obj_resolved = resolve(obj_normal, scope_map);
        auto obj_canon = canonicalize(obj_resolved, "t");
        auto obj_ssa = single_assign(obj_canon, "r");

        auto opt = optimizer(obj_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "let _t0:real = foo();\n"
                                   "let _t1:m^2*Kg^1*s^-3*A^-1 = _t0*3:m^2*Kg^1*s^-3*A^-1;\n"
                                   "let _t5:m^2*Kg^1*s^-3*A^-2 = _t1*0.14285714285714285:A^-1;\n"
                                   "_t5;";

        std::string expanded_ans = "(variable _t5\n"
                                   "  (*\n"
                                   "    (variable _t1\n"
                                   "      (*\n"
                                   "        (variable _t0\n"
                                   "          (call foo))\n"
                                   "        (3)))\n"
                                   "    (0.14285714285714285)))";

        EXPECT_EQ(expected_opt, pretty_print(let_opt));

        auto var = std::get_if<resolved_let>(let_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
    {
        in_scope_map scope_map;
        scope_map.func_map.insert(
                {"foo", make_rexpr<resolved_function>("foo", std::vector<r_expr>{}, real_body, real_type, loc)});

        std::string p_expr = "let a = 1 + 2 + 3;"
                             "a + 4";

        auto p = parser(p_expr);
        auto obj = p.parse_let();

        auto obj_normal = normalize(obj);
        auto obj_resolved = resolve(obj_normal, scope_map);
        auto obj_canon = canonicalize(obj_resolved, "t");
        auto obj_ssa = single_assign(obj_canon, "r");

        std::string expected_opt = "let _t0:real = 1:real+2:real;\n"
                                   "let _t1:real = _t0+3:real;\n"
                                   "let a:real = _t1;\n"
                                   "let _t2:real = a+4:real;\n"
                                   "_t2;";

        std::string expanded_ans = "(variable _t2\n"
                                   "  (+\n"
                                   "    (variable a\n"
                                   "      (variable _t1\n"
                                   "        (+\n"
                                   "          (variable _t0\n"
                                   "            (+\n"
                                   "              (1)\n"
                                   "              (2)))\n"
                                   "          (3))))\n"
                                   "    (4)))";

        EXPECT_EQ(expected_opt, pretty_print(obj_ssa));

        auto var = std::get_if<resolved_let>(obj_ssa.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
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
        scope_map.local_map.insert({"t", make_rexpr<resolved_argument>("t", real_type, loc)});
        scope_map.local_map.insert({"q", make_rexpr<resolved_argument>("q", real_type, loc)});
        scope_map.func_map.insert({"foo", make_rexpr<resolved_function>("foo", std::vector<r_expr>{}, real_body, real_type, loc)});
        scope_map.type_map.insert({"foo", foo_type});
        scope_map.type_map.insert({"bar", bar_type});

        std::string p_expr = "let B:bar = {X = t + (5 - q); Y = {a = 2[V]; b = foo()*1[A];};};\n"
                             "with B.Y;\n"
                             "let r = a/b;\n"
                             "with B;\n"
                             "X*r/3;\n";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "let _t0:real = 5:real-q;\n"
                                   "let _t1:real = t+_t0;\n"
                                   "let _t2:real = foo();\n"
                                   "let _t3:A^1 = _t2*1:A^1;\n"
                                   "let _t10:m^2*Kg^1*s^-3*A^-2 = 2:m^2*Kg^1*s^-3*A^-1/_t3;\n"
                                   "let _t13:m^2*Kg^1*s^-3*A^-2 = _t1*_t10;\n"
                                   "let _t14:m^2*Kg^1*s^-3*A^-2 = _t13*0.33333333333333331:real;\n"
                                   "_t14;";

        std::string expanded_ans = "(variable _t14\n"
                                   "  (*\n"
                                   "    (variable _t13\n"
                                   "      (*\n"
                                   "        (variable _t1\n"
                                   "          (+\n"
                                   "            (argument t)\n"
                                   "            (variable _t0\n"
                                   "              (-\n"
                                   "                (5)\n"
                                   "                (argument q)))))\n"
                                   "        (variable _t10\n"
                                   "          (/\n"
                                   "            (2)\n"
                                   "            (variable _t3\n"
                                   "              (*\n"
                                   "                (variable _t2\n"
                                   "                  (call foo))\n"
                                   "                (1)))))))\n"
                                   "    (0.33333333333333331)))";

        EXPECT_EQ(expected_opt, pretty_print(let_opt));

        auto var = std::get_if<resolved_let>(let_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"t",  make_rexpr<resolved_argument>("t", real_type, loc)});
        scope_map.local_map.insert({"q",  make_rexpr<resolved_argument>("q", real_type, loc)});
        scope_map.func_map.insert({"foo", make_rexpr<resolved_function>("foo", std::vector<r_expr>{}, real_body, real_type, loc)});
        scope_map.type_map.insert({"foo", foo_type});
        scope_map.type_map.insert({"bar", bar_type});

        std::string p_expr = "let B:bar = {X = t + (5 - q); Y = {a = 2[V]; b = foo()*1[A];};};\n"
                             "with B;\n"
                             "with B.Y;\n"
                             "let r = a/b;\n"
                             "let B:foo = {a = 3[V]; b=-2[A];};\n"
                             "with B;\n"
                             "X*r/a;\n";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "let _t0:real = 5:real-q;\n"
                                   "let _t1:real = t+_t0;\n"
                                   "let _t2:real = foo();\n"
                                   "let _t3:A^1 = _t2*1:A^1;\n"
                                   "let _t12:m^2*Kg^1*s^-3*A^-2 = 2:m^2*Kg^1*s^-3*A^-1/_t3;\n"
                                   "let _t17:m^2*Kg^1*s^-3*A^-2 = _t1*_t12;\n"
                                   "let _t18:A^-1 = _t17*0.33333333333333331:m^-2*Kg^-1*s^3*A^1;\n"
                                   "_t18;";

        std::string expanded_ans = "(variable _t18\n"
                                   "  (*\n"
                                   "    (variable _t17\n"
                                   "      (*\n"
                                   "        (variable _t1\n"
                                   "          (+\n"
                                   "            (argument t)\n"
                                   "            (variable _t0\n"
                                   "              (-\n"
                                   "                (5)\n"
                                   "                (argument q)))))\n"
                                   "        (variable _t12\n"
                                   "          (/\n"
                                   "            (2)\n"
                                   "            (variable _t3\n"
                                   "              (*\n"
                                   "                (variable _t2\n"
                                   "                  (call foo))\n"
                                   "                (1)))))))\n"
                                   "    (0.33333333333333331)))";

        EXPECT_EQ(expected_opt, pretty_print(let_opt));

        auto var = std::get_if<resolved_let>(let_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
    {
        in_scope_map scope_map;
        scope_map.type_map.insert({"foo", foo_type});

        std::string p_expr = "let A:foo = {a = 2[V]; b = 0.5[A];};\n"
                             "with A;\n"
                             "let a = a/b;\n"
                             "with A;\n"
                             "a;\n";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "2:m^2*Kg^1*s^-3*A^-1";
        EXPECT_EQ(expected_opt, pretty_print(let_opt));
    }
    {
        in_scope_map scope_map;
        scope_map.type_map.insert({"foo", foo_type});

        std::string p_expr = "let A:foo = {a = 2[V]; b = 0.5[A];};\n"
                             "with A;\n"
                             "let a = a/b;\n"
                             "a;\n";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "4:m^2*Kg^1*s^-3*A^-2";
        EXPECT_EQ(expected_opt, pretty_print(let_opt));
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
        scope_map.local_map.insert({"t", make_rexpr<resolved_argument>("t", real_type, loc)});

        std::string p_expr = "if t == 4 then let a=3; a*4 else 15.5;";

        auto p = parser(p_expr);
        auto ifstmt = p.parse_conditional();

        auto if_normal = normalize(ifstmt);
        auto if_resolved = resolve(if_normal, scope_map);
        auto if_canon = canonicalize(if_resolved, "t");
        auto if_ssa = single_assign(if_canon, "r");

        auto opt = optimizer(if_ssa);
        auto if_opt = opt.optimize();

        std::string expected_opt = "let _t0:bool = t==4:real;\n"
                                   "let _t2:real = _t0? 12:real: 15.5:real;\n"
                                   "_t2;";

        std::string expanded_ans = "(variable _t2\n"
                                   "  (conditional\n"
                                   "    (variable _t0\n"
                                   "      (==\n"
                                   "        (argument t)\n"
                                   "        (4)))\n"
                                   "    (12)\n"
                                   "    (15.5)))";

        EXPECT_EQ(expected_opt, pretty_print(if_opt));

        auto var = std::get_if<resolved_let>(if_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
    {
        in_scope_map scope_map;
        scope_map.func_map.insert({"foo", make_rexpr<resolved_function>("foo", std::vector<r_expr>{}, real_body, real_type, loc)});
        scope_map.type_map.insert({"foo", foo_type});
        scope_map.type_map.insert({"bar", bar_type});
        scope_map.local_map.insert({"t",    make_rexpr<resolved_argument>("t", real_type, loc)});
        scope_map.local_map.insert({"a",    make_rexpr<resolved_argument>("a", real_type, loc)});
        scope_map.local_map.insert({"obar", make_rexpr<resolved_argument>("obar", bar_type, loc)});

        std::string p_expr = "if (if t == 4 then a>3 else a<4)\n"
                             "then (if obar.X == 5 then obar.Y else {a=3[V]; b=5[mA];})\n"
                             "else {a=foo()*3[V]; b=7000[mA];});";

        auto p = parser(p_expr);
        auto ifstmt = p.parse_conditional();

        auto if_normal = normalize(ifstmt);
        auto if_resolved = resolve(if_normal, scope_map);
        auto if_canon = canonicalize(if_resolved, "t");
        auto if_ssa = single_assign(if_canon, "r");

        auto opt = optimizer(if_ssa);
        auto if_opt = opt.optimize();

        std::string expected_opt = "let _t0:bool = t==4:real;\n"
                                   "let _t1:bool = a>3:real;\n"
                                   "let _t2:bool = a<4:real;\n"
                                   "let _t3:bool = _t0? _t1: _t2;\n"
                                   "let _t4:real = obar.X;\n"
                                   "let _t5:bool = _t4==5:real;\n"
                                   "let _t6:{a:m^2*Kg^1*s^-3*A^-1; b:A^1;} = obar.Y;\n"
                                   "let _t8:{a:m^2*Kg^1*s^-3*A^-1; b:A^1;} = "
                                   "_t5? _t6: {a = 3:m^2*Kg^1*s^-3*A^-1; b = 0.0050000000000000001:A^1;};\n"
                                   "let _t9:real = foo();\n"
                                   "let _t10:m^2*Kg^1*s^-3*A^-1 = _t9*3:m^2*Kg^1*s^-3*A^-1;\n"
                                   "let _t12:{a:m^2*Kg^1*s^-3*A^-1; b:A^1;} = "
                                   "_t3? _t8: {a = _t10; b = 7:A^1;};\n"
                                   "_t12;";

        std::string expanded_ans = "(variable _t12\n"
                                   "  (conditional\n"
                                   "    (variable _t3\n"
                                   "      (conditional\n"
                                   "        (variable _t0\n"
                                   "          (==\n"
                                   "            (argument t)\n"
                                   "            (4)))\n"
                                   "        (variable _t1\n"
                                   "          (>\n"
                                   "            (argument a)\n"
                                   "            (3)))\n"
                                   "        (variable _t2\n"
                                   "          (<\n"
                                   "            (argument a)\n"
                                   "            (4)))))\n"
                                   "    (variable _t8\n"
                                   "      (conditional\n"
                                   "        (variable _t5\n"
                                   "          (==\n"
                                   "            (variable _t4\n"
                                   "              (access\n"
                                   "                (argument obar)\n"
                                   "                (X)))\n"
                                   "            (5)))\n"
                                   "        (variable _t6\n"
                                   "          (access\n"
                                   "            (argument obar)\n"
                                   "            (Y)))\n"
                                   "        (object\n"
                                   "          (variable a\n"
                                   "            (3))\n"
                                   "          (variable b\n"
                                   "            (0.0050000000000000001)))))\n"
                                   "    (object\n"
                                   "      (variable a\n"
                                   "        (variable _t10\n"
                                   "          (*\n"
                                   "            (variable _t9\n"
                                   "              (call foo))\n"
                                   "            (3))))\n"
                                   "      (variable b\n"
                                   "        (7)))))";

        EXPECT_EQ(expected_opt, pretty_print(if_opt));

        auto var = std::get_if<resolved_let>(if_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
}

TEST(cse, let) {
    auto loc = src_location{};
    auto real_type    = make_rtype<resolved_quantity>(normalized_type(quantity::real), loc);
    auto current_type = make_rtype<resolved_quantity>(normalized_type(quantity::current), loc);
    auto voltage_type = make_rtype<resolved_quantity>(normalized_type(quantity::voltage), loc);
    auto conductance_type = make_rtype<resolved_quantity>(normalized_type(quantity::conductance), loc);
    auto real_body    = make_rexpr<resolved_float>(0, real_type, loc);

    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"a", make_rexpr<resolved_argument>("a", voltage_type, loc)});
        scope_map.local_map.insert({"s", make_rexpr<resolved_argument>("s", conductance_type, loc)});

        std::string p_expr = "let b:voltage = a + a*5; let c:current = b*s; c*(a*5))";
        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "let _t0:m^2*Kg^1*s^-3*A^-1 = a*5:real;\n"
                                   "let _t1:m^2*Kg^1*s^-3*A^-1 = a+_t0;\n"
                                   "let _t2:A^1 = _t1*s;\n"
                                   "let _t4:m^2*Kg^1*s^-3 = _t2*_t0;\n"
                                   "_t4;";

        std::string expanded_ans = "(variable _t4\n"
                                   "  (*\n"
                                   "    (variable _t2\n"
                                   "      (*\n"
                                   "        (variable _t1\n"
                                   "          (+\n"
                                   "            (argument a)\n"
                                   "            (variable _t0\n"
                                   "              (*\n"
                                   "                (argument a)\n"
                                   "                (5)))))\n"
                                   "        (argument s)))\n"
                                   "    (variable _t0\n"
                                   "      (*\n"
                                   "        (argument a)\n"
                                   "        (5)))))";

        EXPECT_EQ(expected_opt, pretty_print(let_opt));

        auto var = std::get_if<resolved_let>(let_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"a",  make_rexpr<resolved_argument>("a", voltage_type, loc)});
        scope_map.local_map.insert({"s",  make_rexpr<resolved_argument>("s", conductance_type, loc)});

        std::vector<r_expr> args = {make_rexpr<resolved_argument>("a", current_type, loc)};
        auto foo =  make_rexpr<resolved_function>("foo", args, real_body, real_type, loc);
        scope_map.func_map.insert({"foo", foo});


        std::string p_expr = "let b = let x = a+5 [mV] /2; x*s; let c = foo(b)*foo(a*s); c/(foo(b)*1 [A]);";
        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "let _t1:m^2*Kg^1*s^-3*A^-1 = a+0.0025000000000000001:m^2*Kg^1*s^-3*A^-1;\n"
                                   "let _t2:A^1 = _t1*s;\n"
                                   "let _t3:real = foo(_t2);\n"
                                   "let _t4:A^1 = a*s;\n"
                                   "let _t5:real = foo(_t4);\n"
                                   "let _t6:real = _t3*_t5;\n"
                                   "let _t8:A^1 = _t3*1:A^1;\n"
                                   "let _t9:A^-1 = _t6/_t8;\n"
                                   "_t9;";

        std::string expanded_ans = "(variable _t9\n"
                                   "  (/\n"
                                   "    (variable _t6\n"
                                   "      (*\n"
                                   "        (variable _t3\n"
                                   "          (call foo\n"
                                   "            (variable _t2\n"
                                   "              (*\n"
                                   "                (variable _t1\n"
                                   "                  (+\n"
                                   "                    (argument a)\n"
                                   "                    (0.0025000000000000001)))\n"
                                   "                (argument s)))))\n"
                                   "        (variable _t5\n"
                                   "          (call foo\n"
                                   "            (variable _t4\n"
                                   "              (*\n"
                                   "                (argument a)\n"
                                   "                (argument s)))))))\n"
                                   "    (variable _t8\n"
                                   "      (*\n"
                                   "        (variable _t3\n"
                                   "          (call foo\n"
                                   "            (variable _t2\n"
                                   "              (*\n"
                                   "                (variable _t1\n"
                                   "                  (+\n"
                                   "                    (argument a)\n"
                                   "                    (0.0025000000000000001)))\n"
                                   "                (argument s)))))\n"
                                   "        (1)))))";

        EXPECT_EQ(expected_opt, pretty_print(let_opt));

        auto var = std::get_if<resolved_let>(let_opt.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
    {
        in_scope_map scope_map;

        std::string p_expr = "let a = 1; let b = a + 5; let c = a + 5; c;";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "6:real";
        EXPECT_EQ(expected_opt, pretty_print(let_opt));
    }
    {
        in_scope_map scope_map;

        std::string p_expr =
                "let a = 1;\n"
                "let b = min(a, 2);\n"
                "let c = a + b * 5;\n"
                "let d = if (a != 1) then c else b;\n"
                "d;";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::string expected_opt = "1:real";
        EXPECT_EQ(expected_opt, pretty_print(let_opt));
    }
}

TEST(function_inline, misc) {
    auto loc = src_location{};
    auto real_type    = make_rtype<resolved_quantity>(normalized_type(quantity::real), loc);
    {
        std::string bar_func =
            "function bar(a:real) {\n"
            "  let x = 2*a;\n"
            "  x^2;\n"
            "};";

        std::string foo_func =
            "function foo(a:real, b:real, c:real) {\n"
            "  let x = a+b+c;\n"
            "  let y = x*c;\n"
            "  let z = bar(y);\n"
            "  let w = z*x;\n"
            "  w;\n"
            "};";

        std::string let_expr =
            "let a = foo(x, y, z); a;\n";

        // bar
        in_scope_map bar_scope_map;

        auto p_bar = parser(bar_func);
        auto bar = p_bar.parse_function();

        auto bar_normal = normalize(bar);
        auto bar_resolved = resolve(bar_normal, bar_scope_map);
        auto bar_canon = canonicalize(bar_resolved, "t");
        auto bar_ssa = single_assign(bar_canon, "r");

        auto b_opt = optimizer(bar_ssa);
        auto bar_opt = b_opt.optimize();

        // foo
        in_scope_map foo_scope_map;
        foo_scope_map.func_map.insert({"bar", bar_opt});

        auto p_foo = parser(foo_func);
        auto foo = p_foo.parse_function();

        auto foo_normal = normalize(foo);
        auto foo_resolved = resolve(foo_normal, foo_scope_map);
        auto foo_canon = canonicalize(foo_resolved, "t");
        auto foo_ssa = single_assign(foo_canon, "r");

        auto f_opt = optimizer(foo_ssa);
        auto foo_opt = f_opt.optimize();

        // let
        in_scope_map let_scope_map;
        let_scope_map.local_map.insert({"x",  make_rexpr<resolved_argument>("x", real_type, loc)});
        let_scope_map.local_map.insert({"y",  make_rexpr<resolved_argument>("y", real_type, loc)});
        let_scope_map.local_map.insert({"z",  make_rexpr<resolved_argument>("z", real_type, loc)});
        let_scope_map.func_map.insert({"bar", bar_opt});
        let_scope_map.func_map.insert({"foo", foo_opt});


        auto p_let = parser(let_expr);
        auto let = p_let.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, let_scope_map);
        auto let_canon = canonicalize(let_resolved, "t");
        auto let_ssa = single_assign(let_canon, "r");

        auto l_opt = optimizer(let_ssa);
        auto let_opt = l_opt.optimize();

        std::unordered_map<std::string, r_expr> avail_funcs = {{"foo", foo_opt}, {"bar", bar_opt}};
        auto let_inlined = inline_func(let_opt, avail_funcs, "f");

        auto l_opt2 = optimizer(let_inlined);
        auto let_opt2 = l_opt2.optimize();

        std::string expected_opt = "let _t0:real = x+y;\n"
                                   "let _t1:real = _t0+z;\n"
                                   "let _t2:real = _t1*z;\n"
                                   "let _f0:real = 2:real*_t2;\n"
                                   "let _f1:real = _f0^2:real;\n"
                                   "let _t4:real = _f1*_t1;\n"
                                   "_t4;";

        std::string expanded_ans = "(variable _t4\n"
                                   "  (*\n"
                                   "    (variable _f1\n"
                                   "      (^\n"
                                   "        (variable _f0\n"
                                   "          (*\n"
                                   "            (2)\n"
                                   "            (variable _t2\n"
                                   "              (*\n"
                                   "                (variable _t1\n"
                                   "                  (+\n"
                                   "                    (variable _t0\n"
                                   "                      (+\n"
                                   "                        (argument x)\n"
                                   "                        (argument y)))\n"
                                   "                    (argument z)))\n"
                                   "                (argument z)))))\n"
                                   "        (2)))\n"
                                   "    (variable _t1\n"
                                   "      (+\n"
                                   "        (variable _t0\n"
                                   "          (+\n"
                                   "            (argument x)\n"
                                   "            (argument y)))\n"
                                   "        (argument z)))))";

        EXPECT_EQ(expected_opt, pretty_print(let_opt2));

        auto var = std::get_if<resolved_let>(let_opt2.get());
        EXPECT_EQ(expanded_ans, expand(get_innermost_body(var)));
    }
}

TEST(optimizer, mechanism) {
    {
        std::string mech =
            "# Adapted from Allen Institute Ca_dynamics.mod,\n"
            "# in turn based on model of Destexhe et al. 1994.\n"
            "\n"
            "mechanism concentration \"CaDynamics\" {\n"
            "    parameter gamma = 0.05;      # Proportion of unbuffered calcium.\n"
            "    parameter decay = 80 [ms];   # Calcium removal time constant.\n"
            "    parameter minCai = 1e-4 [mM];\n"
            "    parameter depth = 0.1 [um];  # Depth of shell.\n"
            "\n"
            "    bind flux = molar_flux(\"ca\");\n"
            "    bind cai = internal_concentration(\"ca\");\n"
            "    \n"
            "    effect molar_flux(\"ca\") = -(gamma*flux - depth*(cai - minCai)/decay);\n"
            "}";

        auto p = parser(mech);
        auto m = p.parse_mechanism();
        auto m_normal = normalize(m);
        auto m_resolved = resolve(m_normal);
        auto m_canon = canonicalize(m_resolved);
        auto m_ssa = single_assign(m_canon);

        auto opt_0 = optimizer(m_ssa);
        auto m_opt = opt_0.optimize();

        auto m_inlined = inline_func(m_opt);

        auto opt_1 = optimizer(m_inlined);
        auto m_fin = opt_1.optimize();

        std::string expected_opt = "CaDynamics concentration {\n"
                                   "bind flux:m^-2*s^-1*mol^1 = molar_flux[ca];\n"
                                   "bind cai:m^-3*mol^1 = internal_concentration[ca];\n"
                                   "effect molar_flux[ca]:m^-2*s^-1*mol^1 =\n"
                                   "let _t0:m^-2*s^-1*mol^1 = 0.050000000000000003:real*flux;\n"
                                   "let _t1:m^-3*mol^1 = cai-1.0000000000000001e-07:m^-3*mol^1;\n"
                                   "let _t2:m^-2*mol^1 = 9.9999999999999995e-08:m^1*_t1;\n"
                                   "let _t3:m^-2*s^-1*mol^1 = _t2*12.5:s^-1;\n"
                                   "let _t4:m^-2*s^-1*mol^1 = _t0-_t3;\n"
                                   "let _t5:m^-2*s^-1*mol^1 = -_t4;\n"
                                   "_t5;\n"
                                   "}";

        EXPECT_EQ(expected_opt, pretty_print(m_fin));
    }
    {
        std::string mech =
                "mechanism point \"expsyn_stdp\" {\n"
                "    # A scaling factor for incoming (pre-synaptic) events is required, as the\n"
                "    # weight of an event is dimensionelss.\n"
                "\n"
                "    parameter A     =  1 [uS];    # pre-synaptic event contribution factor\n"
                "    parameter Apre  =  0.01 [uS]; # pre-synaptic event plasticity contribution\n"
                "    parameter Apost = -0.01 [uS]; # post-synaptic event plasticity contribution\n"
                "\n"
                "    parameter t      = 2 [ms]; # synaptic time constant\n"
                "    parameter tpre  = 10 [ms]; # pre-synaptic plasticity contrib time constant\n"
                "    parameter tpost = 10 [ms]; # post-synaptic plasticity contrib time constant\n"
                "\n"
                "    parameter gmax  = 10 [uS]; # maximum synaptic conductance\n"
                "    parameter e = 0 [mV];      # reversal potential\n"
                "\n"
                "    bind ca = internal_concentration(\"the other, other calcium\");\n"
                "\n"
                "    record state_rec {\n"
                "        g:         conductance,\n"
                "        apre:      conductance,\n"
                "        apost:     conductance,\n"
                "        w_plastic: conductance\n"
                "    }\n"
                "    state expsyn: state_rec;\n"
                "    state other: volume^2*force;\n"
                "\n"
                "    bind v = membrane_potential;\n"
                "\n"
                "    function external_spike(s: state_rec, weight: real) : state_rec {\n"
                "       let x = s.g + s.w_plastic + weight*A;\n"
                "       let g = if x < 0 [S] then 0 [S]\n"
                "               else if x > gmax then gmax\n"
                "               else x;\n"
                "       let apre = s.apre + Apre; \n"
                "       let w_plastic = s.w_plastic + s.apost;\n"
                "       state_rec {\n"
                "           g = g;\n"
                "           apre = apre; \n"
                "           apost = s.apost; \n"
                "           w_plastic = w_plastic;\n"
                "       }\n"
                "    }\n"
                "\n"
                "    initial expsyn = state_rec {\n"
                "        g         = 0 [uS];\n"
                "        apre      = 0 [uS];\n"
                "        apost     = 0 [uS];\n"
                "        w_plastic = 0 [uS];\n"
                "    };\n"
                "\n"
                "    evolve expsyn' = state_rec' {\n"
                "        g' = -expsyn.g/t;\n"
                "        apre' = -expsyn.apre/tpre;\n"
                "        apost'= -expsyn.apost/tpost;\n"
                "        w_plastic' = 0[uS/s];\n"
                "    };\n"
                "\n"
                "    effect current = expsyn.g*(v - e);\n"
                "}";
        auto p = parser(mech);
        auto m = p.parse_mechanism();
        auto m_normal = normalize(m);
        auto m_resolved = resolve(m_normal);
        auto m_canon = canonicalize(m_resolved);
        auto m_ssa = single_assign(m_canon);

        auto opt_0 = optimizer(m_ssa);
        auto m_opt = opt_0.optimize();

        auto m_inlined = inline_func(m_opt);

        auto opt_1 = optimizer(m_inlined);
        auto m_fin = opt_1.optimize();

        std::string expected_opt =
            "expsyn_stdp point {\n"
            "state expsyn:{"
            "g:m^-2*Kg^-1*s^3*A^2; "
            "apre:m^-2*Kg^-1*s^3*A^2; "
            "apost:m^-2*Kg^-1*s^3*A^2; "
            "w_plastic:m^-2*Kg^-1*s^3*A^2;"
            "};\n"
            "bind v:m^2*Kg^1*s^-3*A^-1 = membrane_potential;\n"
            "initial expsyn:{"
            "g:m^-2*Kg^-1*s^3*A^2; "
            "apre:m^-2*Kg^-1*s^3*A^2; "
            "apost:m^-2*Kg^-1*s^3*A^2; "
            "w_plastic:m^-2*Kg^-1*s^3*A^2;"
            "} =\n"
            "{"
            "g = 0:m^-2*Kg^-1*s^3*A^2; "
            "apre = 0:m^-2*Kg^-1*s^3*A^2; "
            "apost = 0:m^-2*Kg^-1*s^3*A^2; "
            "w_plastic = 0:m^-2*Kg^-1*s^3*A^2;"
            "};\n"
            "evolve expsyn:{"
            "g':m^-2*Kg^-1*s^2*A^2; "
            "apre':m^-2*Kg^-1*s^2*A^2; "
            "apost':m^-2*Kg^-1*s^2*A^2; "
            "w_plastic':m^-2*Kg^-1*s^2*A^2;"
            "} =\n"
            "let _t0:m^-2*Kg^-1*s^3*A^2 = expsyn.g;\n"
            "let _t1:m^-2*Kg^-1*s^3*A^2 = -_t0;\n"
            "let _t2:m^-2*Kg^-1*s^2*A^2 = _t1*500:s^-1;\n"
            "let _t3:m^-2*Kg^-1*s^3*A^2 = expsyn.apre;\n"
            "let _t4:m^-2*Kg^-1*s^3*A^2 = -_t3;\n"
            "let _t5:m^-2*Kg^-1*s^2*A^2 = _t4*100:s^-1;\n"
            "let _t6:m^-2*Kg^-1*s^3*A^2 = expsyn.apost;\n"
            "let _t7:m^-2*Kg^-1*s^3*A^2 = -_t6;\n"
            "let _t8:m^-2*Kg^-1*s^2*A^2 = _t7*100:s^-1;\n"
            "{"
            "g' = _t2; "
            "apre' = _t5; "
            "apost' = _t8; "
            "w_plastic' = 0:m^-2*Kg^-1*s^2*A^2;"
            "};\n"
            "effect current:A^1 =\n"
            "let _t0:m^-2*Kg^-1*s^3*A^2 = expsyn.g;\n"
            "let _t2:A^1 = _t0*v;\n"
            "_t2;\n"
            "}";

        EXPECT_EQ(expected_opt, pretty_print(m_fin));
    }
    {
        std::string mech =
                "mechanism density \"Kd\" {\n"
                "    parameter gbar = 1e-5 [S/cm^2];\n"
                "    parameter ek = -77 [mV];\n"
                "    bind v = membrane_potential;\n"
                "\n"
                "    record state_rec {\n"
                "        m: real,\n"
                "        h: real,\n"
                "    };\n"
                "    state s: state_rec;\n"
                "\n"
                "    function mInf(v: voltage): real {\n"
                "        1 - 1/(1 + exp((v + 43 [mV])/8 [mV]))\n"
                "    };\n"
                "\n"
                "    function hInf(v: voltage): real {\n"
                "        1/(1 + exp((v + 67 [mV])/7.3 [mV]));\n"
                "    }\n"
                "\n"
                "    function state0(v: voltage): state_rec {\n"
                "        state_rec {\n"
                "            m = mInf(v);\n"
                "            h = hInf(v);\n"
                "        };\n"
                "    };\n"
                "\n"
                "    function rate(s: state_rec, v: voltage): state_rec' {\n"
                "        state_rec'{\n"
                "            m' = (s.m - mInf(v))/1 [ms];\n"
                "            h' = (s.h - hInf(v))/1500 [ms];\n"
                "        };\n"
                "    }\n"
                "\n"
                "    function curr(s: state_rec, v_minus_ek: voltage): current/area {\n"
                "        gbar*s.m*s.h*v_minus_ek;\n"
                "    }\n"
                "\n"
                "    initial s = state0(v);\n"
                "    evolve s' = rate(s, v);\n"
                "    effect current_density(\"k\") = curr(s, v - ek);\n"
                "    \n"
                "    export gbar; \n"
                "}";
        auto p = parser(mech);
        auto m = p.parse_mechanism();
        auto m_normal = normalize(m);
        auto m_resolved = resolve(m_normal);
        auto m_canon = canonicalize(m_resolved);
        auto m_ssa = single_assign(m_canon);

        auto opt_0 = optimizer(m_ssa);
        auto m_opt = opt_0.optimize();

        auto m_inlined = inline_func(m_opt);

        auto opt_1 = optimizer(m_inlined);
        auto m_fin = opt_1.optimize();

        std::string expected_opt =
            "Kd density {\n"
            "parameter gbar:m^-4*Kg^-1*s^3*A^2 =\n"
            "0.10000000000000001:m^-4*Kg^-1*s^3*A^2;\n"
            "state s:{m:real; h:real;};\n"
            "bind v:m^2*Kg^1*s^-3*A^-1 = membrane_potential;\n"
            "initial s:{m:real; h:real;} =\n"
            "let _t0:m^2*Kg^1*s^-3*A^-1 = v+0.043000000000000003:m^2*Kg^1*s^-3*A^-1;\n"
            "let _t1:real = _t0*125:m^-2*Kg^-1*s^3*A^1;\n"
            "let _t2:real = exp(_t1);\n"
            "let _t3:real = 1:real+_t2;\n"
            "let _t4:real = 1:real/_t3;\n"
            "let _t5:real = 1:real-_t4;\n"
            "let _f1:m^2*Kg^1*s^-3*A^-1 = v+0.067000000000000004:m^2*Kg^1*s^-3*A^-1;\n"
            "let _f2:real = _f1*136.98630136986301:m^-2*Kg^-1*s^3*A^1;\n"
            "let _f3:real = exp(_f2);\n"
            "let _f4:real = 1:real+_f3;\n"
            "let _f5:real = 1:real/_f4;\n"
            "{m = _t5; h = _f5;};\n"
            "evolve s:{m':s^-1; h':s^-1;} =\n"
            "let _t0:real = s.m;\n"
            "let _f0:m^2*Kg^1*s^-3*A^-1 = v+0.043000000000000003:m^2*Kg^1*s^-3*A^-1;\n"
            "let _t1:real = _f0*125:m^-2*Kg^-1*s^3*A^1;\n"
            "let _t2:real = exp(_t1);\n"
            "let _t3:real = 1:real+_t2;\n"
            "let _t4:real = 1:real/_t3;\n"
            "let _t5:real = 1:real-_t4;\n"
            "let _f2:real = _t0-_t5;\n"
            "let _f3:s^-1 = _f2*1000:s^-1;\n"
            "let _f4:real = s.h;\n"
            "let _f5:m^2*Kg^1*s^-3*A^-1 = v+0.067000000000000004:m^2*Kg^1*s^-3*A^-1;\n"
            "let _f6:real = _f5*136.98630136986301:m^-2*Kg^-1*s^3*A^1;\n"
            "let _f7:real = exp(_f6);\n"
            "let _f8:real = 1:real+_f7;\n"
            "let _f9:real = 1:real/_f8;\n"
            "let _t6:real = _f4-_f9;\n"
            "let _t7:s^-1 = _t6*0.66666666666666663:s^-1;\n"
            "{m' = _f3; h' = _t7;};\n"
            "effect current_density[k]:m^-2*A^1 =\n"
            "let _t0:m^2*Kg^1*s^-3*A^-1 = v--0.076999999999999999:m^2*Kg^1*s^-3*A^-1;\n"
            "let _f0:real = s.m;\n"
            "let _t1:m^-4*Kg^-1*s^3*A^2 = gbar*_f0;\n"
            "let _t2:real = s.h;\n"
            "let _t3:m^-4*Kg^-1*s^3*A^2 = _t1*_t2;\n"
            "let _t4:m^-2*A^1 = _t3*_t0;\n"
            "_t4;\n"
            "export gbar:m^-4*Kg^-1*s^3*A^2;\n"
            "}";

        EXPECT_EQ(expected_opt, pretty_print(m_fin));

        m_fin = solve(m_fin);
        std::cout << pretty_print(m_fin) << std::endl;

        auto m_printable = printable_mechanism(m_fin);
        std::cout << "/**********************************************/" << std::endl;
        std::cout << print_header(m_printable, "namespace").str() << std::endl;
    }
}