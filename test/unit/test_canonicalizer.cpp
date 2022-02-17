#include <cmath>
#include <string>
#include <variant>
#include <vector>

#include <arblang/optimizer/optimizer.hpp>
#include <arblang/parser/token.hpp>
#include <arblang/parser/parser.hpp>
#include <arblang/parser/normalizer.hpp>
#include <arblang/resolver/canonicalize.hpp>
#include <arblang/resolver/resolved_expressions.hpp>
#include <arblang/resolver/resolved_types.hpp>
#include <arblang/resolver/single_assign.hpp>
#include "arblang/util/custom_hash.hpp"

#include "../gtest.h"

using namespace al;
using namespace resolved_ir;
using namespace resolved_type_ir;

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

        std::string p_expr = "foo()";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        auto call_ssa = single_assign(call_canon);

        auto opt = optimizer(call_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t0_:real = foo();
        // t0_;
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", real_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        scope_map.func_map.insert({"foo2", resolved_function("foo2", {arg0, arg1}, real_body, real_type, loc)});

        std::string p_expr = "foo2(2, 1)";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        auto call_ssa = single_assign(call_canon);

        auto opt = optimizer(call_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t0_:foo2(2, 1);
        // t0_;
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", real_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        auto arg2 = make_rexpr<resolved_argument>("c", current_type, loc);
        scope_map.func_map.insert({"foo_bar", resolved_function("foo_bar", {arg0, arg1, arg2}, real_body, real_type, loc)});

        std::string p_expr = "foo_bar(2.5, a, -1 [A])";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        auto call_ssa = single_assign(call_canon);

        auto opt = optimizer(call_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t1_ = foo_bar(2.5, a, -1);
        // t1_;
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", real_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        scope_map.func_map.insert({"foo", resolved_function("foo", {}, real_body, real_type, loc)});
        scope_map.func_map.insert({"bar", resolved_function("bar", {arg0, arg1}, real_body, real_type, loc)});

        std::string p_expr = "bar(1+4, foo())";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        auto call_ssa = single_assign(call_canon);

        auto opt = optimizer(call_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t1_:real = foo();
        // let t2_:real = bar(5, t1_);
        // t2_;
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", voltage_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        scope_map.func_map.insert({"baz", resolved_function("baz", {arg0, arg1}, real_body, real_type, loc)});

        std::string p_expr = "baz(let b: voltage = 6 [mV]; b, bar.X)";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        auto call_ssa = single_assign(call_canon);

        auto opt = optimizer(call_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t0_:real = bar.X;
        // let t1_:real = baz(0.006, t0_);
        // t1_;
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
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // 11:real
    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"a", resolved_argument("a", voltage_type, loc)});
        scope_map.local_map.insert({"s", resolved_argument("s", conductance_type, loc)});

        std::string p_expr = "let b:voltage = a + a*5; let c:current = b*s; c*a*b)";
        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t0_:voltage = a*5;
        // let t1_:voltage = a+t0_; // b
        // let t2_:current = t1_*s; // c
        // let t3_:power = t2_*a;   // c*a
        // let t4_:power*voltage = t3_*t1_; // c*a*b
        // t4_;
    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"a",  resolved_argument("a", voltage_type, loc)});
        scope_map.local_map.insert({"s",  resolved_argument("s", conductance_type, loc)});
        scope_map.func_map.insert({"foo", resolved_function("foo",
                                                            {make_rexpr<resolved_argument>("a", current_type, loc)},
                                                            real_body, real_type, loc)});

        std::string p_expr = "let b = let x = a+5 [mV] /2; x*s; let c = foo(b)*foo(a*s); c/2.1 [A];";
        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t1_:voltage = a+0.0025; // x
        // let t2_:current = t1_*s;    // b
        // let t3_:real = foo(t2_);    // foo(b)
        // let t4_:current = a*s;      //
        // let t5_:real = foo(t4_);    // foo(a*s)
        // let t6_:real = t3_*t5_;     // c
        // let t7_:current = t6_/2.1;  // c/2.1
        // t7_;

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

        std::string p_expr = "let B:bar = {X = t + (5 - q); Y = {a = 2[V]; b = foo()*1[A];};};\n"
                             "with B.Y;\n"
                             "let r = a/b;\n"
                             "with B;\n"
                             "X*r/3;\n";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t0_:real = 5-q;
        // let t1_:real = t+t0_;                                                     // B.X
        // let t2_:real = foo();
        // let t3_:current = t2_*1;                                                  // B.Y.b
        // let t10_:resistance = 2/t3_;                                              // r
        // let t13_:resistance = t1_*t10_;                                           // B.X*r
        // let t14_:resistance = t13_/3;                                             // B.X*r/3
        // t14_;
    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"t", resolved_argument("t", real_type, loc)});
        scope_map.local_map.insert({"q", resolved_argument("q", real_type, loc)});
        scope_map.func_map.insert({"foo", resolved_function("foo", {}, real_body, real_type, loc)});
        scope_map.type_map.insert({"foo", foo_type});
        scope_map.type_map.insert({"bar", bar_type});

        std::string p_expr = "let B:bar = {X = t + (5 - q); Y = {a = 2[V]; b = foo()*1[A];};};\n"
                             "with B;\n"
                             "with B.Y;\n"
                             "let r = a/b;\n"
                             "let B:foo = {a = 3[V]; b=-2[A];};\n"
                             "with B;\n"
                             "X*r/3;\n";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t0_:real = 5-q;
        // let t1_:real = t+t0_;              // t1_ -> B.X
        // let t2_:real = foo();
        // let t3_:current = t2_*1;           // t3_ -> B.Y.b
        // let t12_:resistance = 2/t3_;       // t12_ -> r
        // let t17_:resistance = t1_*t12_;    // B.X*r
        // let t18_:resistance = t17_/3;      // B.X*r/3
        // t18_;
    }
    {
        in_scope_map scope_map;
        scope_map.type_map.insert({"foo", foo_type});

        std::string p_expr = "let A:foo = {a = 2[V]; b = 1[A];};\n"
                             "with A;\n"
                             "let a = a/b;\n"
                             "with A;\n"
                             "a;\n";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t0_:{a:voltage; b:current} = {a=2; b=1;};
        // let t1_:voltage = t0_.a;
        // t1_;
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

        std::string p_expr = "if t == 4 then let a=3; a*4 else 15.5;";

        auto p = parser(p_expr);
        auto ifstmt = p.parse_conditional();

        auto if_normal = normalize(ifstmt);
        auto if_resolved = resolve(if_normal, scope_map);
        auto if_canon = canonicalize(if_resolved);
        auto if_ssa = single_assign(if_canon);

        auto opt = optimizer(if_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t0_:bool = t==4;
        // let t2_:real = t0_? 12: 15.5;
        // t2_;
    }
    {
        in_scope_map scope_map;
        scope_map.func_map.insert({"foo", resolved_function("foo", {}, real_body, real_type, loc)});
        scope_map.type_map.insert({"foo", foo_type});
        scope_map.type_map.insert({"bar", bar_type});
        scope_map.local_map.insert({"t", resolved_argument("t", real_type, loc)});
        scope_map.local_map.insert({"a", resolved_argument("a", real_type, loc)});
        scope_map.local_map.insert({"obar", resolved_argument("obar", bar_type, loc)});

        std::string p_expr = "if (if t == 4 then a>3 else a<4)\n"
                             "then (if obar.X == 5 then obar.Y else {a=3[V]; b=5[mA];})\n"
                             "else {a=foo()*3[V]; b=7000[mA];});";

        auto p = parser(p_expr);
        auto ifstmt = p.parse_conditional();

        auto if_normal = normalize(ifstmt);
        auto if_resolved = resolve(if_normal, scope_map);
        auto if_canon = canonicalize(if_resolved);
        auto if_ssa = single_assign(if_canon);

        auto opt = optimizer(if_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t0_:bool = t==4;
        // let t1_:real = a>3;
        // let t2_:bool = a<4;
        // let t3_:bool = t0_? t1_: t2_;                     // if t == 4 then a>3 else a<4
        // let t4_:real = obar.X;
        // let t5_:bool = t4_==5;
        // let t6_:{a:voltage; b:current} = obar.Y;
        // let t7_:{a:voltage; b:current} = {a=3; b=0.005};
        // let t8_:{a:voltage; b:current} = t5_? t6_: t7_;   // if obar.X == 5 then obar.Y else {a=3[V]; b=5[mA];}
        // let t9_:real = foo();
        // let t10_:voltage = t9_*3;
        // let t11_:{a:voltage; b:current} = {a=t10_; b=7;}
        // let t12_:{a:voltage; b:current} = t3_? t8_: t11_;
        // t12_;
    }
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

    std::cout << std::endl;
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
        scope_map.local_map.insert({"a", resolved_argument("a", voltage_type, loc)});
        scope_map.local_map.insert({"s", resolved_argument("s", conductance_type, loc)});

        std::string p_expr = "let b:voltage = a + a*5; let c:current = b*s; c*(a*5))";
        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t0_:voltage = a*5;
        // let t1_:voltage = a+t0_; // b
        // let t2_:current = t1_*s; // c
        // let t4_:power = t2_*t0_;
        // t4_;

    }
    {
        in_scope_map scope_map;
        scope_map.local_map.insert({"a",  resolved_argument("a", voltage_type, loc)});
        scope_map.local_map.insert({"s",  resolved_argument("s", conductance_type, loc)});
        scope_map.func_map.insert({"foo", resolved_function("foo",
                                                            {make_rexpr<resolved_argument>("a", current_type, loc)},
                                                            real_body, real_type, loc)});

        std::string p_expr = "let b = let x = a+5 [mV] /2; x*s; let c = foo(b)*foo(a*s); c/(foo(b) * 1 [A]);";
        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // let t1_:voltage = a+0.0025[V]; // x
        // let t2_:current = t1_*s;       // b
        // let t3_:real = foo(t2_);       // foo(b)
        // let t4_:current = a*s;
        // let t5_:real = foo(t4_);       // foo(a*s)
        // let t6_:real = t3_*t5_;        // c
        // let t8_:current = t3_*1[A];    // foo(b)*1[A]
        // let t9_:a/current = t6_/t8_;   // c/
        // t9_

    }
    {
        in_scope_map scope_map;

        std::string p_expr = "let a = 1; let b = a + 5; let c = a + 5; c;";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // 6:real
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
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        while (opt.keep_optimizing()) {
            opt.optimize();
        }
        std::cout << to_string(opt.expression()) << std::endl;
        std::cout << std::endl;

        // 1:real
    }
}