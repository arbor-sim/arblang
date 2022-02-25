#include <cmath>
#include <string>
#include <variant>
#include <vector>

#include <arblang/optimizer/optimizer.hpp>
#include <arblang/optimizer/inline_func.hpp>
#include <arblang/parser/token.hpp>
#include <arblang/parser/parser.hpp>
#include <arblang/parser/normalizer.hpp>
#include <arblang/resolver/canonicalize.hpp>
#include <arblang/resolver/resolve.hpp>
#include <arblang/resolver/resolved_expressions.hpp>
#include <arblang/resolver/resolved_types.hpp>
#include <arblang/resolver/single_assign.hpp>
#include <arblang/util/custom_hash.hpp>

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
        auto call_canon = canonicalize(call_resolved);
        auto call_ssa = single_assign(call_canon);

        auto opt = optimizer(call_ssa);
        auto call_opt = opt.optimize();

        std::cout << to_string(call_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(call_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // let t0_:real = foo();
        // t0_;
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
        auto call_canon = canonicalize(call_resolved);
        auto call_ssa = single_assign(call_canon);

        auto opt = optimizer(call_ssa);
        auto call_opt = opt.optimize();

        std::cout << to_string(call_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(call_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // let t0_:foo2(2, 1);
        // t0_;
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
        auto call_canon = canonicalize(call_resolved);
        auto call_ssa = single_assign(call_canon);

        auto opt = optimizer(call_ssa);
        auto call_opt = opt.optimize();

        std::cout << to_string(call_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(call_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // let t1_ = foo_bar(2.5, a, -1);
        // t1_;
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
        auto call_canon = canonicalize(call_resolved);
        auto call_ssa = single_assign(call_canon);

        auto opt = optimizer(call_ssa);
        auto call_opt = opt.optimize();

        std::cout << to_string(call_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(call_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }
        // let t1_:real = foo();
        // let t2_:real = bar(5, t1_);
        // t2_;
    }
    {
        auto arg0 = make_rexpr<resolved_argument>("a", voltage_type, loc);
        auto arg1 = make_rexpr<resolved_argument>("b", real_type, loc);
        scope_map.func_map.insert({"baz", make_rexpr<resolved_function>("baz", std::vector<r_expr>{arg0, arg1}, real_body, real_type, loc)});

        std::string p_expr = "baz(let b: voltage = 6 [mV]; b, bar.X)";
        auto p = parser(p_expr);
        auto call = p.parse_call();

        auto call_normal = normalize(call);
        auto call_resolved = resolve(call_normal, scope_map);
        auto call_canon = canonicalize(call_resolved);
        auto call_ssa = single_assign(call_canon);

        auto opt = optimizer(call_ssa);
        auto call_opt = opt.optimize();

        std::cout << to_string(call_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(call_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

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

        std::cout << to_string(let_ssa) << std::endl;
        std::cout << std::endl;

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // 11:real
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
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // let t0_:voltage = a*5;
        // let t1_:voltage = a+t0_; // b
        // let t2_:current = t1_*s; // c
        // let t3_:power = t2_*a;   // c*a
        // let t4_:power*voltage = t3_*t1_; // c*a*b
        // t4_;
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
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

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
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

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
                             "X*r/3;\n";

        auto p = parser(p_expr);
        auto let = p.parse_let();

        auto let_normal = normalize(let);
        auto let_resolved = resolve(let_normal, scope_map);
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

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

        std::string p_expr = "let A:foo = {a = 2[V]; b = 0.5[A];};\n"
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
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // 2:voltage
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
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto let = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(let), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // 4:resistance
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
        auto if_canon = canonicalize(if_resolved);
        auto if_ssa = single_assign(if_canon);

        auto opt = optimizer(if_ssa);
        auto if_opt = opt.optimize();

        std::cout << to_string(if_opt) << std::endl;
        std::cout << std::endl;
        if (auto if_stmt = std::get_if<resolved_let>(if_opt.get())) {
            std::cout << to_string(get_innermost_body(if_stmt), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // let t0_:bool = t==4;
        // let t2_:real = t0_? 12: 15.5;
        // t2_;
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
        auto if_canon = canonicalize(if_resolved);
        auto if_ssa = single_assign(if_canon);

        auto opt = optimizer(if_ssa);
        auto if_opt = opt.optimize();

        std::cout << to_string(if_opt) << std::endl;
        std::cout << std::endl;
        if (auto if_stmt = std::get_if<resolved_let>(if_opt.get())) {
            std::cout << to_string(get_innermost_body(if_stmt), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        /// TODO buggy
        // let t0_:bool = t==4;
        // let t1_:bool = a>3;
        // let t2_:bool = a<4;
        // let t3_:bool = t0_? t1_: t2_;                     // if t == 4 then a>3 else a<4
        // let t4_:real = obar.X;
        // let t5_:bool = t4_==5;
        // let t6_:{a:voltage; b:current} = obar.Y;
        // let t8_:{a:voltage; b:current} = t5_? t6_: {a=3; b=0.005};   // if obar.X == 5 then obar.Y else {a=3[V]; b=5[mA];}
        // let t9_:real = foo();
        // let t10_:voltage = t9_*3;
        // let t12_:{a:voltage; b:current} = t3_? t8_: {a=t10_; b=7;};
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
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto if_stmt = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(if_stmt), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // let t0_:voltage = a*5;
        // let t1_:voltage = a+t0_; // b
        // let t2_:current = t1_*s; // c
        // let t4_:power = t2_*t0_;
        // t4_;

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
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto opt = optimizer(let_ssa);
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto if_stmt = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(if_stmt), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // let t1_:voltage = a+0.0025[V]; // x
        // let t2_:current = t1_*s;       // b
        // let t3_:real = foo(t2_);       // foo(b)
        // let t4_:current = a*s;
        // let t5_:real = foo(t4_);       // foo(a*s)
        // let t6_:real = t3_*t5_;        // c
        // let t8_:current = t3_*1[A];    // foo(b)*1[A]
        // let t9_:a/current = t6_/t8_;   // c/foo(b)*1[A]
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
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto if_stmt = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(if_stmt), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

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
        auto let_opt = opt.optimize();

        std::cout << to_string(let_opt) << std::endl;
        std::cout << std::endl;
        if (auto if_stmt = std::get_if<resolved_let>(let_opt.get())) {
            std::cout << to_string(get_innermost_body(if_stmt), false, true, 0) << std::endl;
            std::cout << std::endl;
        }

        // 1:real
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
        auto bar_canon = canonicalize(bar_resolved);
        auto bar_ssa = single_assign(bar_canon);

        auto b_opt = optimizer(bar_ssa);
        auto bar_opt = b_opt.optimize();

        // foo
        in_scope_map foo_scope_map;
        foo_scope_map.func_map.insert({"bar", bar_opt});

        auto p_foo = parser(foo_func);
        auto foo = p_foo.parse_function();

        auto foo_normal = normalize(foo);
        auto foo_resolved = resolve(foo_normal, foo_scope_map);
        auto foo_canon = canonicalize(foo_resolved);
        auto foo_ssa = single_assign(foo_canon);

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
        auto let_canon = canonicalize(let_resolved);
        auto let_ssa = single_assign(let_canon);

        auto l_opt = optimizer(let_ssa);
        auto let_opt = l_opt.optimize();

        std::unordered_map<std::string, r_expr> avail_funcs = {{"foo", foo_opt}, {"bar", bar_opt}};
        auto let_inlined = inline_func(let_opt, avail_funcs);

        auto l_opt2 = optimizer(let_inlined);
        auto let_opt2 = l_opt2.optimize();

        std::cout << to_string(let_opt2) << std::endl;
        std::cout << std::endl;
        if (auto ll = std::get_if<resolved_let>(let_opt2.get())) {
            std::cout << to_string(get_innermost_body(ll), false, true, 0) << std::endl;
            std::cout << std::endl;
        }
    }

    // let _t0:real = x+y;
    // let _t1:real = _t0+z;   // foo::x
    // let _t2:real = _t1*z;   // foo::y
    // let _r0:real = 2*_t2;   // bar::x
    // let _r1:real = _r0^2;   // bar
    // let _t4:real = _r1*_t1; // foo
    // _t4;
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

        std::cout << "-----------------------------" << std::endl;
        std::cout << mech << std::endl;
        std::cout << "-----------------------------" << std::endl;
        std::cout << to_string(m_fin) << std::endl;
        std::cout << "-----------------------------" << std::endl;
        std::cout << std::endl;
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

        std::cout << "-----------------------------" << std::endl;
        std::cout << mech << std::endl;
        std::cout << "-----------------------------" << std::endl;
        std::cout << to_string(m_fin) << std::endl;
        std::cout << "-----------------------------" << std::endl;
        std::cout << std::endl;
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

        std::cout << "-----------------------------" << std::endl;
        std::cout << mech << std::endl;
        std::cout << "-----------------------------" << std::endl;
        std::cout << to_string(m_fin) << std::endl;
        std::cout << "-----------------------------" << std::endl;
        std::cout << std::endl;
    }
}

/// TODO bugs:
/// object fields aren't being resolved correctly (durng opt)