#include <cmath>
#include <string>
#include <variant>
#include <vector>

#include <arblang/parser/token.hpp>
#include <arblang/parser/parser.hpp>
#include <arblang/parser/normalizer.hpp>
#include <arblang/resolver/resolved_expressions.hpp>

#include "../gtest.h"

using namespace al;
using namespace parsed_ir;

// TODO test exceptions properly

TEST(parser, unit) {
    using namespace parsed_unit_ir;
    {
        std::string unit = "[mV]";
        auto p = parser(unit);
        auto u = std::get<parsed_simple_unit>(*p.try_parse_unit());
        EXPECT_EQ(unit_pref::m, u.val.prefix);
        EXPECT_EQ(unit_sym::V, u.val.symbol);
    }
    {
        std::string unit = "[mmol/kA]";
        auto p = parser(unit);
        auto u = std::get<parsed_binary_unit>(*p.try_parse_unit());
        auto lhs = std::get<parsed_simple_unit>(*u.lhs);
        auto rhs = std::get<parsed_simple_unit>(*u.rhs);

        EXPECT_EQ(unit_pref::m, lhs.val.prefix);
        EXPECT_EQ(unit_sym::mol, lhs.val.symbol);

        EXPECT_EQ(unit_pref::k, rhs.val.prefix);
        EXPECT_EQ(unit_sym::A, rhs.val.symbol);

        EXPECT_EQ(u_binary_op::div, u.op);
    }
    {
        std::string unit = "[K^-2]";
        auto p = parser(unit);
        auto u = std::get<parsed_binary_unit>(*p.try_parse_unit());
        auto lhs = std::get<parsed_simple_unit>(*u.lhs);
        auto rhs = std::get<parsed_integer_unit>(*u.rhs);

        EXPECT_EQ(unit_pref::none, lhs.val.prefix);
        EXPECT_EQ(unit_sym::K, lhs.val.symbol);

        EXPECT_EQ(-2, rhs.val);

        EXPECT_EQ(u_binary_op::pow, u.op);
    }
    {
        std::string unit = "[Ohm*uV/YS]";
        auto p = parser(unit);
        auto u = std::get<parsed_binary_unit>(*p.try_parse_unit());

        EXPECT_EQ(u_binary_op::div, u.op);
        auto lhs_0 = std::get<parsed_binary_unit>(*u.lhs); // Ohm*uV
        auto rhs_0 = std::get<parsed_simple_unit>(*u.rhs); // YS

        EXPECT_EQ(u_binary_op::mul, lhs_0.op);
        auto lhs_1 = std::get<parsed_simple_unit>(*lhs_0.lhs); // Ohm
        auto rhs_1 = std::get<parsed_simple_unit>(*lhs_0.rhs); // uV

        EXPECT_EQ(unit_pref::none, lhs_1.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_1.val.symbol);

        EXPECT_EQ(unit_pref::u, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::V, rhs_1.val.symbol);

        EXPECT_EQ(unit_pref::Y, rhs_0.val.prefix);
        EXPECT_EQ(unit_sym::S, rhs_0.val.symbol);
    }
    {
        std::string unit = "[Ohm^2/daC/K^-3]";
        auto p = parser(unit);
        auto u = std::get<parsed_binary_unit>(*p.try_parse_unit());

        EXPECT_EQ(u_binary_op::div, u.op);
        auto lhs_0 = std::get<parsed_binary_unit>(*u.lhs); // Ohm^2/daC
        auto rhs_0 = std::get<parsed_binary_unit>(*u.rhs); // K^-3

        EXPECT_EQ(u_binary_op::div, lhs_0.op);
        auto lhs_1 = std::get<parsed_binary_unit>(*lhs_0.lhs); // Ohm^2
        auto rhs_1 = std::get<parsed_simple_unit>(*lhs_0.rhs); // daC

        EXPECT_EQ(u_binary_op::pow, lhs_1.op);
        auto lhs_2 = std::get<parsed_simple_unit>(*lhs_1.lhs);  // Ohm
        auto rhs_2 = std::get<parsed_integer_unit>(*lhs_1.rhs); // 2

        EXPECT_EQ(u_binary_op::pow, rhs_0.op);
        auto lhs_3 = std::get<parsed_simple_unit>(*rhs_0.lhs);  // K
        auto rhs_3 = std::get<parsed_integer_unit>(*rhs_0.rhs); // -3

        EXPECT_EQ(unit_pref::da, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::C, rhs_1.val.symbol);

        EXPECT_EQ(unit_pref::none, lhs_2.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_2.val.symbol);

        EXPECT_EQ(2, rhs_2.val);

        EXPECT_EQ(unit_pref::none, lhs_3.val.prefix);
        EXPECT_EQ(unit_sym::K, lhs_3.val.symbol);

        EXPECT_EQ(-3, rhs_3.val);
    }
    {
        std::string unit = "[Ohm^2/daC*mK^-1]";
        auto p = parser(unit);
        auto u = std::get<parsed_binary_unit>(*p.try_parse_unit());

        EXPECT_EQ(u_binary_op::mul, u.op);
        auto lhs_0 = std::get<parsed_binary_unit>(*u.lhs); // Ohm^2/daC
        auto rhs_0 = std::get<parsed_binary_unit>(*u.rhs); // K^-3

        EXPECT_EQ(u_binary_op::div, lhs_0.op);
        auto lhs_1 = std::get<parsed_binary_unit>(*lhs_0.lhs); // Ohm^2
        auto rhs_1 = std::get<parsed_simple_unit>(*lhs_0.rhs); // daC

        EXPECT_EQ(u_binary_op::pow, lhs_1.op);
        auto lhs_2 = std::get<parsed_simple_unit>(*lhs_1.lhs);  // Ohm
        auto rhs_2 = std::get<parsed_integer_unit>(*lhs_1.rhs); // 2

        EXPECT_EQ(u_binary_op::pow, rhs_0.op);
        auto lhs_3 = std::get<parsed_simple_unit>(*rhs_0.lhs);  // K
        auto rhs_3 = std::get<parsed_integer_unit>(*rhs_0.rhs); // -3

        EXPECT_EQ(unit_pref::da, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::C, rhs_1.val.symbol);

        EXPECT_EQ(unit_pref::none, lhs_2.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_2.val.symbol);

        EXPECT_EQ(2, rhs_2.val);

        EXPECT_EQ(unit_pref::m, lhs_3.val.prefix);
        EXPECT_EQ(unit_sym::K, lhs_3.val.symbol);

        EXPECT_EQ(-1, rhs_3.val);
    }
    {
        std::string unit = "[(Ohm/V)*A]";
        auto p = parser(unit);
        auto u = std::get<parsed_binary_unit>(*p.try_parse_unit());

        EXPECT_EQ(u_binary_op::mul, u.op);
        auto lhs_0 = std::get<parsed_binary_unit>(*u.lhs); // Ohm/V
        auto rhs_0 = std::get<parsed_simple_unit>(*u.rhs); // A

        EXPECT_EQ(u_binary_op::div, lhs_0.op);
        auto lhs_1 = std::get<parsed_simple_unit>(*lhs_0.lhs); // Ohm
        auto rhs_1 = std::get<parsed_simple_unit>(*lhs_0.rhs); // V

        EXPECT_EQ(unit_pref::none, lhs_1.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_1.val.symbol);

        EXPECT_EQ(unit_pref::none, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::V, rhs_1.val.symbol);

        EXPECT_EQ(unit_pref::none, rhs_0.val.prefix);
        EXPECT_EQ(unit_sym::A, rhs_0.val.symbol);
    }
    {
        std::string unit = "[Ohm/(V*A)]";
        auto p = parser(unit);
        auto u = std::get<parsed_binary_unit>(*p.try_parse_unit());

        EXPECT_EQ(u_binary_op::div, u.op);
        auto lhs_0 = std::get<parsed_simple_unit>(*u.lhs); // Ohm
        auto rhs_0 = std::get<parsed_binary_unit>(*u.rhs); // V*A

        EXPECT_EQ(u_binary_op::mul, rhs_0.op);
        auto lhs_1 = std::get<parsed_simple_unit>(*rhs_0.lhs); // V
        auto rhs_1 = std::get<parsed_simple_unit>(*rhs_0.rhs); // A

        EXPECT_EQ(unit_pref::none, lhs_0.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_0.val.symbol);

        EXPECT_EQ(unit_pref::none, lhs_1.val.prefix);
        EXPECT_EQ(unit_sym::V, lhs_1.val.symbol);

        EXPECT_EQ(unit_pref::none, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::A, rhs_1.val.symbol);
    }
    {
        std::vector<std::string> invalid_units = {
            "[Ohm^A]",
            "[2^uK]",
            "[pV^(2/mV)]",
            "[-Ohm]",
            "[4.5*Ohm]",
            "[+V]",
            "[identifier]",
            "[7]"
        };
        for (auto u: invalid_units) {
            auto p = parser(u);
            EXPECT_THROW(p.try_parse_unit(), std::runtime_error);
        }
    }
    {
        std::string iden = "identifier";
        auto p = parser(iden);
        auto unit = p.try_parse_unit();
        EXPECT_TRUE(std::get_if<parsed_no_unit>(unit.get()));
    }
}

TEST(parse, type) {
    {
        std::string type = "time";
        auto p = parser(type);
        auto q = std::get<parsed_quantity_type>(*p.parse_type());
        EXPECT_EQ(quantity::time, q.type);
    }
    {
        std::string type = "bar";
        auto p = parser(type);
        auto q = std::get<parsed_record_alias_type>(*p.parse_type());
        EXPECT_EQ("bar", q.name);
    }
    {
        std::string type = "voltage^2";
        auto p = parser(type);
        auto q = std::get<parsed_binary_quantity_type>(*p.parse_type());
        EXPECT_EQ(t_binary_op::pow, q.op);

        auto q_lhs = std::get<parsed_quantity_type>(*q.lhs);
        EXPECT_EQ(quantity::voltage, q_lhs.type);

        auto q_rhs = std::get<parsed_integer_type>(*q.rhs);
        EXPECT_EQ(2, q_rhs.val);
    }
    {
        std::string type = "voltage/conductance";
        auto p = parser(type);
        auto q = std::get<parsed_binary_quantity_type>(*p.parse_type());
        EXPECT_EQ(t_binary_op::div, q.op);

        auto q_lhs = std::get<parsed_quantity_type>(*q.lhs);
        EXPECT_EQ(quantity::voltage, q_lhs.type);

        auto q_rhs = std::get<parsed_quantity_type>(*q.rhs);
        EXPECT_EQ(quantity::conductance, q_rhs.type);
    }
    {
        std::string type = "current*time/voltage";
        auto p = parser(type);
        auto q = std::get<parsed_binary_quantity_type>(*p.parse_type());
        EXPECT_EQ(t_binary_op::div, q.op);

        auto q0 = std::get<parsed_binary_quantity_type>(*q.lhs);
        EXPECT_EQ(t_binary_op::mul, q0.op);

        auto q0_lhs = std::get<parsed_quantity_type>(*q0.lhs);
        EXPECT_EQ(quantity::current, q0_lhs.type);

        auto q0_rhs = std::get<parsed_quantity_type>(*q0.rhs);
        EXPECT_EQ(quantity::time, q0_rhs.type);

        auto q_rhs = std::get<parsed_quantity_type>(*q.rhs);
        EXPECT_EQ(quantity::voltage, q_rhs.type);
    }
    {
        std::string type = "current^-2/temperature*time^-1";
        auto p = parser(type);
        auto q = std::get<parsed_binary_quantity_type>(*p.parse_type());
        EXPECT_EQ(t_binary_op::mul, q.op);

        auto q0 = std::get<parsed_binary_quantity_type>(*q.lhs); // current^-2/temperature
        EXPECT_EQ(t_binary_op::div, q0.op);

        auto q1 = std::get<parsed_binary_quantity_type>(*q0.lhs); // current^-2
        EXPECT_EQ(t_binary_op::pow, q1.op);

        auto q1_lhs = std::get<parsed_quantity_type>(*q1.lhs); // current
        EXPECT_EQ(quantity::current, q1_lhs.type);

        auto q1_rhs = std::get<parsed_integer_type>(*q1.rhs); // -2
        EXPECT_EQ(-2, q1_rhs.val);

        auto q0_rhs = std::get<parsed_quantity_type>(*q0.rhs); // temperature
        EXPECT_EQ(quantity::temperature, q0_rhs.type);

        auto q2 = std::get<parsed_binary_quantity_type>(*q.rhs); // time^-1
        EXPECT_EQ(t_binary_op::pow, q2.op);

        auto q2_lhs = std::get<parsed_quantity_type>(*q2.lhs); // time
        EXPECT_EQ(quantity::time, q2_lhs.type);

        auto q2_rhs = std::get<parsed_integer_type>(*q2.rhs); // -1
        EXPECT_EQ(-1, q2_rhs.val);
    }
    {
        std::string type = "current^-2/(temperature*time^-1)";
        auto p = parser(type);
        auto q = std::get<parsed_binary_quantity_type>(*p.parse_type());
        EXPECT_EQ(t_binary_op::div, q.op);

        auto q0 = std::get<parsed_binary_quantity_type>(*q.lhs); // current^-2
        EXPECT_EQ(t_binary_op::pow, q0.op);

        auto q0_lhs = std::get<parsed_quantity_type>(*q0.lhs); // current
        EXPECT_EQ(quantity::current, q0_lhs.type);

        auto q0_rhs = std::get<parsed_integer_type>(*q0.rhs); // -2
        EXPECT_EQ(-2, q0_rhs.val);

        auto q1 = std::get<parsed_binary_quantity_type>(*q.rhs); // temperature*time^-1
        EXPECT_EQ(t_binary_op::mul, q1.op);

        auto q1_lhs = std::get<parsed_quantity_type>(*q1.lhs); // temperature
        EXPECT_EQ(quantity::temperature, q1_lhs.type);

        auto q2 = std::get<parsed_binary_quantity_type>(*q1.rhs); // time^-1
        EXPECT_EQ(t_binary_op::pow, q2.op);

        auto q2_lhs = std::get<parsed_quantity_type>(*q2.lhs); // time
        EXPECT_EQ(quantity::time, q2_lhs.type);

        auto q2_rhs = std::get<parsed_integer_type>(*q2.rhs); // -1
        EXPECT_EQ(-1, q2_rhs.val);
    }
    {
        std::string type = "{bar:voltage, baz:current^5}";
        auto p = parser(type);
        auto q = std::get<parsed_record_type>(*p.parse_type());
        EXPECT_EQ(2, q.fields.size());

        EXPECT_EQ("bar", q.fields[0].first);
        EXPECT_EQ("baz", q.fields[1].first);

        auto q0 = std::get<parsed_quantity_type>(*q.fields[0].second);
        EXPECT_EQ(quantity::voltage, q0.type);

        auto q1 = std::get<parsed_binary_quantity_type>(*q.fields[1].second);
        EXPECT_EQ(t_binary_op::pow, q1.op);

        auto q1_lhs = std::get<parsed_quantity_type>(*q1.lhs);
        EXPECT_EQ(quantity::current, q1_lhs.type);

        auto q1_rhs = std::get<parsed_integer_type>(*q1.rhs);
        EXPECT_EQ(5, q1_rhs.val);
    }
    {
        std::vector<std::string> incorrect_types = {
            "voltage^resistance",
            "voltage+2",
            "2^current",
            "power^(temperature^2)",
            "{voltage}",
            "{foo: 2}",
        };
        for (auto u: incorrect_types) {
            auto p = parser(u);
            EXPECT_THROW(p.parse_type(), std::runtime_error);
        }
    }
}

TEST(parser, identifier) {
    {
        std::string identifier = "foo";
        auto p = parser(identifier);
        auto e_iden = std::get<parsed_identifier>(*p.parse_identifier());
        EXPECT_EQ("foo", e_iden.name);
        EXPECT_FALSE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);
    }
    {
        std::string identifier = "voltage";
        auto p = parser(identifier);
        EXPECT_THROW(p.parse_identifier(), std::runtime_error);
    }
}

TEST(parser, typed_identifier) {
    {
        std::string identifier = "foo";
        auto p = parser(identifier);
        auto e_iden = std::get<parsed_identifier>(*p.parse_typed_identifier());
        EXPECT_EQ("foo", e_iden.name);
        EXPECT_FALSE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);
    }
    {
        std::string identifier = "bar:time";
        auto p = parser(identifier);
        auto e_iden = std::get<parsed_identifier>(*p.parse_typed_identifier());
        EXPECT_EQ("bar", e_iden.name);
        EXPECT_TRUE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);

        auto type = std::get<parsed_quantity_type>(*e_iden.type.value());
        EXPECT_EQ(quantity::time, type.type);
        EXPECT_EQ(src_location(1,5), type.loc);
    }
    {
        std::string identifier = "foo: bar";
        auto p = parser(identifier);
        auto e_iden = std::get<parsed_identifier>(*p.parse_typed_identifier());
        EXPECT_EQ("foo", e_iden.name);
        EXPECT_TRUE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);

        auto type = std::get<parsed_record_alias_type>(*e_iden.type.value());
        EXPECT_EQ("bar", type.name);
        EXPECT_EQ(src_location(1,6), type.loc);
    }
    {
        std::string identifier = "foo: {bar:voltage, baz:current/time}";
        auto p = parser(identifier);
        auto e_iden = std::get<parsed_identifier>(*p.parse_typed_identifier());
        EXPECT_EQ("foo", e_iden.name);
        EXPECT_TRUE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);

        auto type = std::get<parsed_record_type>(*e_iden.type.value());
        EXPECT_EQ(2, type.fields.size());
        EXPECT_EQ(src_location(1,6), type.loc);

        EXPECT_EQ("bar", type.fields[0].first);
        EXPECT_EQ("baz", type.fields[1].first);

        auto t_0 = std::get<parsed_quantity_type>(*type.fields[0].second);
        EXPECT_EQ(quantity::voltage, t_0.type);
        EXPECT_EQ(src_location(1, 11), t_0.loc);

        auto t_1 = std::get<parsed_binary_quantity_type>(*type.fields[1].second);
        EXPECT_EQ(t_binary_op::div, t_1.op);
        EXPECT_EQ(src_location(1, 31), t_1.loc);

        auto t_1_0 = std::get<parsed_quantity_type>(*t_1.lhs);
        EXPECT_EQ(quantity::current, t_1_0.type);

        auto t_1_1 = std::get<parsed_quantity_type>(*t_1.rhs);
        EXPECT_EQ(quantity::time, t_1_1.type);
    }
    {
        std::vector<std::string> invalid = {
                "a:1",         // Invalid integer type
                "foo': /time", // Incomplete type
                "bar: ",       // Missing type after :
                "bar: {a; b}", // Invalid record type (identifier+type required)
        };
        for (const auto& s: invalid) {
            auto p = parser(s);
            EXPECT_THROW(p.parse_typed_identifier(), std::runtime_error);
        }
    }
}

TEST(parser, float_pt) {
    {
        std::string fpt = "4.2";
        auto p = parser(fpt);
        auto v = std::get<parsed_float>(*p.parse_float());
        EXPECT_EQ(4.2, v.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(v.unit.get()));
    }
    {
        std::string fpt = "2.22 [mV]";
        auto p = parser(fpt);
        auto v = std::get<parsed_float>(*p.parse_float());
        EXPECT_EQ(2.22, v.value);

        auto u = std::get<parsed_simple_unit>(*v.unit);
        EXPECT_EQ(unit_pref::m, u.val.prefix);
        EXPECT_EQ(unit_sym::V, u.val.symbol);
    }
    {
        std::string fpt = "2e-4 [A/s]";
        auto p = parser(fpt);
        auto v = std::get<parsed_float>(*p.parse_float());
        EXPECT_EQ(2e-4, v.value);

        auto u = std::get<parsed_binary_unit>(*v.unit);
        EXPECT_EQ(u_binary_op::div, u.op);

        auto u_lhs = std::get<parsed_simple_unit>(*u.lhs);
        EXPECT_EQ(unit_pref::none, u_lhs.val.prefix);
        EXPECT_EQ(unit_sym::A, u_lhs.val.symbol);

        auto u_rhs = std::get<parsed_simple_unit>(*u.rhs);
        EXPECT_EQ(unit_pref::none, u_rhs.val.prefix);
        EXPECT_EQ(unit_sym::s, u_rhs.val.symbol);
    }
    {
        std::string fpt = "2E2 [Ohm^2]";
        auto p = parser(fpt);
        auto v = std::get<parsed_float>(*p.parse_float());
        EXPECT_EQ(2e2, v.value);

        auto u = std::get<parsed_binary_unit>(*v.unit);
        EXPECT_EQ(u_binary_op::pow, u.op);

        auto u_lhs = std::get<parsed_simple_unit>(*u.lhs);
        EXPECT_EQ(unit_pref::none, u_lhs.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, u_lhs.val.symbol);

        auto u_rhs = std::get<parsed_integer_unit>(*u.rhs);
        EXPECT_EQ(2, u_rhs.val);
    }
}

TEST(parser, integer) {
    {
        std::string fpt = "4";
        auto p = parser(fpt);
        auto v = std::get<parsed_int>(*p.parse_int());
        EXPECT_EQ(4, v.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(v.unit.get()));
    }
    {
        std::string fpt = "11 [mV]";
        auto p = parser(fpt);
        auto v = std::get<parsed_int>(*p.parse_int());
        EXPECT_EQ(11, v.value);

        auto u = std::get<parsed_simple_unit>(*v.unit);
        EXPECT_EQ(unit_pref::m, u.val.prefix);
        EXPECT_EQ(unit_sym::V, u.val.symbol);
    }
}

TEST(parser, call) {
    {
        std::string p_expr = "foo()";
        auto p = parser(p_expr);
        auto c = std::get<parsed_call>(*p.parse_call());

        EXPECT_EQ("foo", c.function_name);
        EXPECT_EQ(0u, c.call_args.size());
        EXPECT_EQ(src_location(1, 1), c.loc);
    }
    {
        std::string p_expr = "foo(2, 1)";
        auto p = parser(p_expr);
        auto c = std::get<parsed_call>(*p.parse_call());

        EXPECT_EQ("foo", c.function_name);
        EXPECT_EQ(2u, c.call_args.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg_0 = std::get<parsed_int>(*c.call_args[0]);
        EXPECT_EQ(2, arg_0.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(arg_0.unit.get()));
        EXPECT_EQ(src_location(1, 5), arg_0.loc);

        auto arg_1 = std::get<parsed_int>(*c.call_args[1]);
        EXPECT_EQ(1, arg_1.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(arg_1.unit.get()));
        EXPECT_EQ(src_location(1, 8), arg_1.loc);
    }
    {
        std::string p_expr = "foo_bar(2.5, a, -1 [A])";
        auto p = parser(p_expr);
        auto c = std::get<parsed_call>(*p.parse_call());

        EXPECT_EQ("foo_bar", c.function_name);
        EXPECT_EQ(3u, c.call_args.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg_0 = std::get<parsed_float>(*c.call_args[0]);
        EXPECT_EQ(2.5, arg_0.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(arg_0.unit.get()));
        EXPECT_EQ(src_location(1, 9), arg_0.loc);

        auto arg_1 = std::get<parsed_identifier>(*c.call_args[1]);
        EXPECT_EQ("a", arg_1.name);
        EXPECT_FALSE(arg_1.type);
        EXPECT_EQ(src_location(1, 14), arg_1.loc);

        auto arg_2 = std::get<parsed_unary>(*c.call_args[2]);
        EXPECT_EQ(unary_op::neg, arg_2.op);
        EXPECT_EQ(src_location(1, 17), arg_2.loc);

        auto arg_2_v = std::get<parsed_int>(*arg_2.value);
        EXPECT_EQ(1, arg_2_v.value);
        auto unit = std::get<parsed_simple_unit>(*arg_2_v.unit);
        EXPECT_EQ(unit_pref::none, unit.val.prefix);
        EXPECT_EQ(unit_sym::A, unit.val.symbol);
    }
    {
        std::string p_expr = "foo(1+4, bar())";
        auto p = parser(p_expr);
        auto c = std::get<parsed_call>(*p.parse_call());

        EXPECT_EQ("foo", c.function_name);
        EXPECT_EQ(2u, c.call_args.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg0 = std::get<parsed_binary>(*c.call_args[0]);
        EXPECT_EQ(binary_op::add, arg0.op);
        EXPECT_EQ(src_location(1, 6), arg0.loc);

        auto arg0_lhs = std::get<parsed_int>(*arg0.lhs);
        EXPECT_EQ(1, arg0_lhs.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(arg0_lhs.unit.get()));

        auto arg0_rhs = std::get<parsed_int>(*arg0.rhs);
        EXPECT_EQ(4, arg0_rhs.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(arg0_rhs.unit.get()));

        auto arg1 = std::get<parsed_call>(*c.call_args[1]);
        EXPECT_EQ("bar", arg1.function_name);
        EXPECT_EQ(0u, arg1.call_args.size());
        EXPECT_EQ(src_location(1, 10), arg1.loc);
    }
    {
        std::string p_expr = "foo(let b: voltage = 6 [mV]; b, bar.X)";
        auto p = parser(p_expr);
        auto c = std::get<parsed_call>(*p.parse_call());

        EXPECT_EQ("foo", c.function_name);
        EXPECT_EQ(2u, c.call_args.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg0 = std::get<parsed_let>(*c.call_args[0]);
        EXPECT_EQ(src_location(1, 5), arg0.loc);

        auto arg0_id = std::get<parsed_identifier>(*arg0.identifier);
        EXPECT_EQ("b", arg0_id.name);
        EXPECT_TRUE(arg0_id.type);
        auto arg0_type = std::get<parsed_quantity_type>(*(arg0_id.type.value()));
        EXPECT_EQ(quantity::voltage, arg0_type.type);

        auto arg0_v = std::get<parsed_int>(*arg0.value);
        EXPECT_EQ(6, arg0_v.value);
        auto arg0_unit = std::get<parsed_simple_unit>(*arg0_v.unit);
        EXPECT_EQ(unit_pref::m, arg0_unit.val.prefix);
        EXPECT_EQ(unit_sym::V, arg0_unit.val.symbol);

        auto arg0_b = std::get<parsed_identifier>(*arg0.body);
        EXPECT_EQ("b", arg0_b.name);
        EXPECT_FALSE(arg0_b.type);

        auto arg1 = std::get<parsed_binary>(*c.call_args[1]);
        EXPECT_EQ(src_location(1, 36), arg1.loc);
        EXPECT_EQ(binary_op::dot, arg1.op);

        auto arg1_lhs = std::get<parsed_identifier>(*arg1.lhs);
        EXPECT_EQ("bar", arg1_lhs.name);
        EXPECT_FALSE(arg1_lhs.type);

        auto arg1_rhs = std::get<parsed_identifier>(*arg1.rhs);
        EXPECT_EQ("X", arg1_rhs.name);
        EXPECT_FALSE(arg1_rhs.type);
    }
}

TEST(parser, object) {
    {
        std::string obj = "bar{a = 0; b = 0;}";
        auto p = parser(obj);
        auto c = std::get<parsed_object>(*p.parse_object());

        EXPECT_TRUE(c.record_name);
        EXPECT_EQ("bar", c.record_name.value());
        EXPECT_EQ(2u, c.record_fields.size());
        EXPECT_EQ(2u, c.record_values.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg_0 = std::get<parsed_identifier>(*c.record_fields[0]);
        EXPECT_EQ("a", arg_0.name);
        EXPECT_FALSE(arg_0.type);
        auto val_0 = std::get<parsed_int>(*c.record_values[0]);
        EXPECT_EQ(0, val_0.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(val_0.unit.get()));

        auto arg_1 = std::get<parsed_identifier>(*c.record_fields[1]);
        EXPECT_EQ("b", arg_1.name);
        EXPECT_FALSE(arg_1.type);
        auto val_1 = std::get<parsed_int>(*c.record_values[1]);
        EXPECT_EQ(0, val_1.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(val_1.unit.get()));
    }
    {
        std::string obj = "{a = 1; b = 2e-5;}";
        auto p = parser(obj);
        auto c = std::get<parsed_object>(*p.parse_object());

        EXPECT_FALSE(c.record_name);
        EXPECT_EQ(2u, c.record_fields.size());
        EXPECT_EQ(2u, c.record_values.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg_0 = std::get<parsed_identifier>(*c.record_fields[0]);
        EXPECT_EQ("a", arg_0.name);
        EXPECT_FALSE(arg_0.type);
        auto val_0 = std::get<parsed_int>(*c.record_values[0]);
        EXPECT_EQ(1, val_0.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(val_0.unit.get()));

        auto arg_1 = std::get<parsed_identifier>(*c.record_fields[1]);
        EXPECT_EQ("b", arg_1.name);
        EXPECT_FALSE(arg_1.type);
        auto val_1 = std::get<parsed_float>(*c.record_values[1]);
        EXPECT_EQ(2e-5, val_1.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(val_1.unit.get()));
    }
}

TEST(parser, let) {
    {
        std::string let = "let foo = 9; 12.62";
        auto p = parser(let);
        auto e_let = std::get<parsed_let>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e_let.loc);

        auto id = std::get<parsed_identifier>(*e_let.identifier);
        EXPECT_EQ("foo", id.name);
        EXPECT_FALSE(id.type);
        EXPECT_EQ(src_location(1,5), id.loc);

        auto val = std::get<parsed_int>(*e_let.value);
        EXPECT_EQ(9, val.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(val.unit.get()));
        EXPECT_EQ(src_location(1,11), val.loc);

        auto body = std::get<parsed_float>(*e_let.body);
        EXPECT_EQ(12.62, body.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(body.unit.get()));
        EXPECT_EQ(src_location(1,14), body.loc);
    }
    {
        std::string let = "let g' = (exp(-g)); bar";
        auto p = parser(let);
        auto e_let = std::get<parsed_let>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e_let.loc);

        auto id = std::get<parsed_identifier>(*e_let.identifier);
        EXPECT_EQ("g'", id.name);
        EXPECT_FALSE(id.type);
        EXPECT_EQ(src_location(1,5), id.loc);

        auto val = std::get<parsed_unary>(*e_let.value);
        EXPECT_EQ(unary_op::exp, val.op);
        EXPECT_FALSE(val.is_boolean());
        EXPECT_EQ(src_location(1,11), val.loc);

        auto val_0 = std::get<parsed_unary>(*val.value);
        EXPECT_EQ(unary_op::neg, val_0.op);
        EXPECT_FALSE(val_0.is_boolean());
        EXPECT_EQ(src_location(1,15), val_0.loc);

        auto val_1 = std::get<parsed_identifier>(*val_0.value);
        EXPECT_EQ("g", val_1.name);
        EXPECT_FALSE(val_1.type);
        EXPECT_EQ(src_location(1,16), val_1.loc);

        auto body = std::get<parsed_identifier>(*e_let.body);
        EXPECT_EQ("bar", body.name);
        EXPECT_FALSE(body.type);
        EXPECT_EQ(src_location(1,21), body.loc);
    }
    {
        std::string let = "let a:voltage = -5; a + 3e5";
        auto p = parser(let);
        auto e_let = std::get<parsed_let>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e_let.loc);

        auto id = std::get<parsed_identifier>(*e_let.identifier);
        EXPECT_EQ("a", id.name);
        EXPECT_EQ(src_location(1,5), id.loc);

        auto type = std::get<parsed_quantity_type>(*id.type.value());
        EXPECT_EQ(quantity::voltage, type.type);
        EXPECT_EQ(src_location(1,7), type.loc);

        auto val = std::get<parsed_unary>(*e_let.value);
        EXPECT_EQ(unary_op::neg, val.op);
        EXPECT_FALSE(val.is_boolean());
        EXPECT_EQ(src_location(1,17), val.loc);

        auto intval = std::get<parsed_int>(*val.value);
        EXPECT_EQ(5, intval.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(intval.unit.get()));
        EXPECT_EQ(src_location(1,18), intval.loc);

        auto body = std::get<parsed_binary>(*e_let.body);
        EXPECT_EQ(binary_op::add, body.op);
        EXPECT_FALSE(body.is_boolean());

        auto lhs = std::get<parsed_identifier>(*body.lhs);
        EXPECT_EQ("a", lhs.name);
        EXPECT_FALSE(lhs.type);
        EXPECT_EQ(src_location(1,21), lhs.loc);

        auto rhs = std::get<parsed_float>(*body.rhs);
        EXPECT_EQ(3e5, rhs.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(rhs.unit.get()));
        EXPECT_EQ(src_location(1,25), rhs.loc);
    }
    {
        std::string let = "let x = { a = 4; }.a; x";
        auto p = parser(let);
        auto e = std::get<parsed_let>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_id = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ(src_location(1, 5), e_id.loc);
        EXPECT_EQ("x", e_id.name);
        EXPECT_FALSE(e_id.type);

        auto e_val = std::get<parsed_binary>(*e.value);
        EXPECT_EQ(src_location(1, 19), e_val.loc);
        EXPECT_EQ(binary_op::dot, e_val.op);
        EXPECT_FALSE(e_val.is_boolean());

        auto e_val_lhs = std::get<parsed_object>(*e_val.lhs);
        EXPECT_FALSE(e_val_lhs.record_name);
        EXPECT_EQ(1u, e_val_lhs.record_values.size());
        EXPECT_EQ(1u, e_val_lhs.record_fields.size());

        auto e_val_lhs_arg = std::get<parsed_identifier>(*e_val_lhs.record_fields.front());
        EXPECT_EQ("a", e_val_lhs_arg.name);
        EXPECT_FALSE(e_val_lhs_arg.type);

        auto e_val_lhs_val = std::get<parsed_int>(*e_val_lhs.record_values.front());
        EXPECT_EQ(4, e_val_lhs_val.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_val_lhs_val.unit.get()));

        auto e_val_rhs = std::get<parsed_identifier>(*e_val.rhs);
        EXPECT_EQ("a", e_val_rhs.name);
        EXPECT_FALSE(e_val_rhs.type);

        auto e_body = std::get<parsed_identifier>(*e.body);
        EXPECT_EQ(src_location(1, 23), e_body.loc);
        EXPECT_EQ("x", e_body.name);
        EXPECT_FALSE(e_body.type);

    }
    {
        std::string let = "let F = { u = G.t; }; F.u / 20 [s]";
        auto p = parser(let);
        auto e = std::get<parsed_let>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_id = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ("F", e_id.name);
        EXPECT_FALSE(e_id.type);
        EXPECT_EQ(src_location(1,5), e_id.loc);

        auto e_val = std::get<parsed_object>(*e.value);
        EXPECT_EQ(src_location(1,9), e_val.loc);
        EXPECT_FALSE(e_val.record_name);
        EXPECT_EQ(1u, e_val.record_values.size());
        EXPECT_EQ(1u, e_val.record_fields.size());

        auto e_val_arg = std::get<parsed_identifier>(*e_val.record_fields.front());
        EXPECT_EQ("u", e_val_arg.name);
        EXPECT_FALSE(e_val_arg.type);

        auto e_val_val = std::get<parsed_binary>(*e_val.record_values.front());
        EXPECT_EQ(binary_op::dot, e_val_val.op);

        auto e_val_lhs = std::get<parsed_identifier>(*e_val_val.lhs);
        EXPECT_EQ("G", e_val_lhs.name);
        EXPECT_FALSE(e_val_lhs.type);
        auto e_val_rhs = std::get<parsed_identifier>(*e_val_val.rhs);
        EXPECT_EQ("t", e_val_rhs.name);
        EXPECT_FALSE(e_val_rhs.type);

        auto e_body = std::get<parsed_binary>(*e.body);
        EXPECT_EQ(src_location(1,27), e_body.loc);
        EXPECT_EQ(binary_op::div, e_body.op);

        auto e_body_lhs = std::get<parsed_binary>(*e_body.lhs);
        EXPECT_EQ(binary_op::dot, e_body_lhs.op);

        auto e_body_lhs_lhs = std::get<parsed_identifier>(*e_body_lhs.lhs);
        EXPECT_EQ("F", e_body_lhs_lhs.name);
        EXPECT_FALSE(e_body_lhs_lhs.type);

        auto e_body_lhs_rhs = std::get<parsed_identifier>(*e_body_lhs.rhs);
        EXPECT_EQ("u", e_body_lhs_rhs.name);
        EXPECT_FALSE(e_body_lhs_rhs.type);

        auto e_body_rhs = std::get<parsed_int>(*e_body.rhs);
        EXPECT_EQ(20, e_body_rhs.value);

        auto unit = std::get<parsed_simple_unit>(*e_body_rhs.unit);
        EXPECT_EQ(unit_pref::none, unit.val.prefix);
        EXPECT_EQ(unit_sym::s, unit.val.symbol);
    }
    {
        std::string let = "let a = 3 [m];\n"
                          "let r = { a = 4; b = a; };\n"
                          "r.b";

        auto p = parser(let);
        auto e = std::get<parsed_let>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_id = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ(src_location(1,5), e_id.loc);
        EXPECT_EQ("a", e_id.name);
        EXPECT_FALSE(e_id.type);

        auto e_val = std::get<parsed_int>(*e.value);
        EXPECT_EQ(src_location(1,9), e_val.loc);
        EXPECT_EQ(3, e_val.value);
        auto unit = std::get<parsed_simple_unit>(*e_val.unit);
        EXPECT_EQ(unit_pref::none, unit.val.prefix);
        EXPECT_EQ(unit_sym::m, unit.val.symbol);

        auto e_body = std::get<parsed_let>(*e.body);
        EXPECT_EQ(src_location(2,1), e_body.loc);

        auto e_body_id = std::get<parsed_identifier>(*e_body.identifier);
        EXPECT_EQ(src_location(2,5), e_body_id.loc);
        EXPECT_EQ("r", e_body_id.name);
        EXPECT_FALSE(e_body_id.type);

        auto e_body_val = std::get<parsed_object>(*e_body.value);
        EXPECT_EQ(src_location(2,9), e_body_val.loc);
        EXPECT_FALSE(e_body_val.record_name);

        auto e_body_val_arg0 = std::get<parsed_identifier>(*e_body_val.record_fields[0]);
        EXPECT_EQ("a", e_body_val_arg0.name);
        EXPECT_FALSE(e_body_val_arg0.type);

        auto e_body_val_val0 = std::get<parsed_int>(*e_body_val.record_values[0]);
        EXPECT_EQ(4, e_body_val_val0.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_body_val_val0.unit.get()));

        auto e_body_val_arg1 = std::get<parsed_identifier>(*e_body_val.record_fields[1]);
        EXPECT_EQ("b", e_body_val_arg1.name);
        EXPECT_FALSE(e_body_val_arg1.type);

        auto e_body_val_val1 = std::get<parsed_identifier>(*e_body_val.record_values[1]);
        EXPECT_EQ("a", e_body_val_val1.name);
        EXPECT_FALSE(e_body_val_val1.type);

        auto e_body_body = std::get<parsed_binary>(*e_body.body);
        EXPECT_EQ(src_location(3,2), e_body_body.loc);
        EXPECT_EQ(binary_op::dot, e_body_body.op);

        auto e_body_body_lhs = std::get<parsed_identifier>(*e_body_body.lhs);
        EXPECT_EQ("r", e_body_body_lhs.name);
        EXPECT_FALSE(e_body_body_lhs.type);

        auto e_body_body_rhs = std::get<parsed_identifier>(*e_body_body.rhs);
        EXPECT_EQ("b", e_body_body_rhs.name);
        EXPECT_FALSE(e_body_body_rhs.type);
    }
    {
        std::string let = "let a = { scale = 3.2; pos = { x = 3 [m]; y = 4 [m]; }; }; # binds `a` below.\n"
                          "with a.pos;   # binds `x` to 3 m and `y` to 4 m below.\n"
                          "a.scale*(x+y)";

        auto p = parser(let);
        auto e = std::get<parsed_let>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_id = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ("a", e_id.name);
        EXPECT_FALSE(e_id.type);

        auto e_val = std::get<parsed_object>(*e.value);
        EXPECT_EQ(src_location(1,1), e.loc);
        EXPECT_FALSE(e_val.record_name);
        EXPECT_EQ(2u, e_val.record_fields.size());
        EXPECT_EQ(2u, e_val.record_values.size());

        auto e_val_arg0 = std::get<parsed_identifier>(*e_val.record_fields[0]);
        EXPECT_EQ("scale", e_val_arg0.name);
        EXPECT_FALSE(e_val_arg0.type);

        auto e_val_val0 = std::get<parsed_float>(*e_val.record_values[0]);
        EXPECT_EQ(3.2, e_val_val0.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_val_val0.unit.get()));

        auto e_val_arg1 = std::get<parsed_identifier>(*e_val.record_fields[1]);
        EXPECT_EQ("pos", e_val_arg1.name);
        EXPECT_FALSE(e_val_arg1.type);

        auto e_val_val1 = std::get<parsed_object>(*e_val.record_values[1]);
        EXPECT_FALSE(e_val_val1.record_name);
        EXPECT_EQ(2u, e_val_val1.record_fields.size());
        EXPECT_EQ(2u, e_val_val1.record_values.size());

        auto e_val_val1_arg0 = std::get<parsed_identifier>(*e_val_val1.record_fields[0]);
        EXPECT_EQ("x", e_val_val1_arg0.name);
        EXPECT_FALSE(e_val_val1_arg0.type);

        auto e_val_val1_val0 = std::get<parsed_int>(*e_val_val1.record_values[0]);
        EXPECT_EQ(3, e_val_val1_val0.value);
        auto unit0 = std::get<parsed_simple_unit>(*e_val_val1_val0.unit);
        EXPECT_EQ(unit_pref::none, unit0.val.prefix);
        EXPECT_EQ(unit_sym::m, unit0.val.symbol);

        auto e_val_val1_arg1 = std::get<parsed_identifier>(*e_val_val1.record_fields[1]);
        EXPECT_EQ("y", e_val_val1_arg1.name);
        EXPECT_FALSE(e_val_val1_arg1.type);

        auto e_val_val1_val1 = std::get<parsed_int>(*e_val_val1.record_values[1]);
        EXPECT_EQ(4, e_val_val1_val1.value);
        auto unit1 = std::get<parsed_simple_unit>(*e_val_val1_val1.unit);
        EXPECT_EQ(unit_pref::none, unit1.val.prefix);
        EXPECT_EQ(unit_sym::m, unit1.val.symbol);

        auto e_body = std::get<parsed_with>(*e.body);
        EXPECT_EQ(src_location(2,1), e_body.loc);

        auto e_body_val = std::get<parsed_binary>(*e_body.value);
        EXPECT_EQ(src_location(2,7), e_body_val.loc);
        EXPECT_EQ(binary_op::dot, e_body_val.op);

        auto e_body_val_lhs = std::get<parsed_identifier>(*e_body_val.lhs);
        EXPECT_EQ("a", e_body_val_lhs.name);
        EXPECT_FALSE(e_body_val_lhs.type);

        auto e_body_val_rhs = std::get<parsed_identifier>(*e_body_val.rhs);
        EXPECT_EQ("pos", e_body_val_rhs.name);
        EXPECT_FALSE(e_body_val_rhs.type);

        auto e_body_body = std::get<parsed_binary>(*e_body.body);
        EXPECT_EQ(src_location(3,8), e_body_body.loc);
        EXPECT_EQ(binary_op::mul, e_body_body.op);

        auto e_body_body_lhs = std::get<parsed_binary>(*e_body_body.lhs);
        EXPECT_EQ(binary_op::dot, e_body_body_lhs.op);

        auto e_body_body_lhs_lhs = std::get<parsed_identifier>(*e_body_body_lhs.lhs);
        EXPECT_EQ("a", e_body_body_lhs_lhs.name);
        EXPECT_FALSE(e_body_body_lhs_lhs.type);

        auto e_body_body_lhs_rhs = std::get<parsed_identifier>(*e_body_body_lhs.rhs);
        EXPECT_EQ("scale", e_body_body_lhs_rhs.name);
        EXPECT_FALSE(e_body_body_lhs_rhs.type);

        auto e_body_body_rhs = std::get<parsed_binary>(*e_body_body.rhs);
        EXPECT_EQ(binary_op::add, e_body_body_rhs.op);

        auto e_body_body_rhs_lhs = std::get<parsed_identifier>(*e_body_body_rhs.lhs);
        EXPECT_EQ("x", e_body_body_rhs_lhs.name);
        EXPECT_FALSE(e_body_body_rhs_lhs.type);

        auto e_body_body_rhs_rhs = std::get<parsed_identifier>(*e_body_body_rhs.rhs);
        EXPECT_EQ("y", e_body_body_rhs_rhs.name);
        EXPECT_FALSE(e_body_body_rhs_rhs.type);
    }
    {
        std::string let = "let g = let a = v*v; a/p1; g*3";
        auto p = parser(let);
        auto e = std::get<parsed_let>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_id = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ(src_location(1,5), e_id.loc);
        EXPECT_EQ("g", e_id.name);
        EXPECT_FALSE(e_id.type);

        auto e_val = std::get<parsed_let>(*e.value);
        EXPECT_EQ(src_location(1,9), e_val.loc);

        auto e_val_id = std::get<parsed_identifier>(*e_val.identifier);
        EXPECT_EQ("a", e_val_id.name);
        EXPECT_FALSE(e_val_id.type);

        auto e_val_val = std::get<parsed_binary>(*e_val.value);
        EXPECT_EQ(binary_op::mul, e_val_val.op);

        auto e_val_val_lhs = std::get<parsed_identifier>(*e_val_val.lhs);
        EXPECT_EQ("v", e_val_val_lhs.name);
        EXPECT_FALSE(e_val_val_lhs.type);

        auto e_val_val_rhs = std::get<parsed_identifier>(*e_val_val.rhs);
        EXPECT_EQ("v", e_val_val_rhs.name);
        EXPECT_FALSE(e_val_val_rhs.type);

        auto e_val_body = std::get<parsed_binary>(*e_val.body);
        EXPECT_EQ(binary_op::div, e_val_body.op);

        auto e_val_body_lhs = std::get<parsed_identifier>(*e_val_body.lhs);
        EXPECT_EQ("a", e_val_body_lhs.name);
        EXPECT_FALSE(e_val_body_lhs.type);

        auto e_val_body_rhs = std::get<parsed_identifier>(*e_val_body.rhs);
        EXPECT_EQ("p1", e_val_body_rhs.name);
        EXPECT_FALSE(e_val_body_rhs.type);

        auto e_body = std::get<parsed_binary>(*e.body);
        EXPECT_EQ(src_location(1,29), e_body.loc);
        EXPECT_EQ(binary_op::mul, e_body.op);

        auto e_body_lhs = std::get<parsed_identifier>(*e_body.lhs);
        EXPECT_EQ("g", e_body_lhs.name);
        EXPECT_FALSE(e_body_lhs.type);

        auto e_body_rhs = std::get<parsed_int>(*e_body.rhs);
        EXPECT_EQ(3, e_body_rhs.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_body_rhs.unit.get()));
    }
    {
        std::vector<std::string> invalid = {
            "let a:voltage = -5; a + ", // Incomplete body expression
            "let a: = 3; 0",            // Missing type expression
            "let a = -1e5 0",           // Missing semicolon
            "let _foo = 0; 0",          // Invalid identifier name
            "let foo = 0;",             // Missing body
            "let foo = a:voltage; foo + 1", // Invalid typed identifier on rhs of equal
        };
        for (const auto& s: invalid) {
            auto p = parser(s);
            EXPECT_THROW(p.parse_let(), std::runtime_error);
        }
    }
}

TEST(parser, with) {
    {
        std::string with = "with S; a.x";
        auto p = parser(with);
        auto e = std::get<parsed_with>(*p.parse_with());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_val = std::get<parsed_identifier>(*e.value);
        EXPECT_EQ(src_location(1,6), e_val.loc);
        EXPECT_EQ("S", e_val.name);
        EXPECT_FALSE(e_val.type);

        auto e_body = std::get<parsed_binary>(*e.body);
        EXPECT_EQ(src_location(1,10), e_body.loc);
        EXPECT_EQ(binary_op::dot, e_body.op);

        auto e_body_lhs = std::get<parsed_identifier>(*e_body.lhs);
        EXPECT_EQ("a", e_body_lhs.name);
        EXPECT_FALSE(e_body_lhs.type);

        auto e_body_rhs = std::get<parsed_identifier>(*e_body.rhs);
        EXPECT_EQ("x", e_body_rhs.name);
        EXPECT_FALSE(e_body_rhs.type);
    }
    {
        std::string with = "with bar(); x - 1 [s]";
        auto p = parser(with);
        auto e = std::get<parsed_with>(*p.parse_with());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_val = std::get<parsed_call>(*e.value);
        EXPECT_EQ(src_location(1,6), e_val.loc);
        EXPECT_EQ("bar", e_val.function_name);
        EXPECT_EQ(0u, e_val.call_args.size());

        auto e_body = std::get<parsed_binary>(*e.body);
        EXPECT_EQ(src_location(1,15), e_body.loc);
        EXPECT_EQ(binary_op::sub, e_body.op);

        auto e_body_lhs = std::get<parsed_identifier>(*e_body.lhs);
        EXPECT_EQ("x", e_body_lhs.name);
        EXPECT_FALSE(e_body_lhs.type);

        auto e_body_rhs = std::get<parsed_int>(*e_body.rhs);
        EXPECT_EQ(1, e_body_rhs.value);

        auto unit = std::get<parsed_simple_unit>(*e_body_rhs.unit);
        EXPECT_EQ(unit_pref::none, unit.val.prefix);
        EXPECT_EQ(unit_sym::s, unit.val.symbol);
    }
    {
        std::string with = "with {a = {x = 1 [ms];}; b:voltage = 4 [V];}.a; x";
        auto p = parser(with);
        auto e = std::get<parsed_with>(*p.parse_with());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_val = std::get<parsed_binary>(*e.value);
        EXPECT_EQ(src_location(1,45), e_val.loc);
        EXPECT_EQ(binary_op::dot, e_val.op);

        auto e_val_lhs = std::get<parsed_object>(*e_val.lhs);
        EXPECT_FALSE(e_val_lhs.record_name);
        EXPECT_EQ(2u, e_val_lhs.record_fields.size());
        EXPECT_EQ(2u, e_val_lhs.record_values.size());

        auto e_val_lhs_arg0 = std::get<parsed_identifier>(*e_val_lhs.record_fields[0]);
        EXPECT_EQ("a", e_val_lhs_arg0.name);
        EXPECT_FALSE(e_val_lhs_arg0.type);

        auto e_val_lhs_val0 = std::get<parsed_object>(*e_val_lhs.record_values[0]);
        EXPECT_FALSE(e_val_lhs_val0.record_name);
        EXPECT_EQ(1u, e_val_lhs_val0.record_fields.size());
        EXPECT_EQ(1u, e_val_lhs_val0.record_values.size());

        auto e_val_lhs_val0_arg0 = std::get<parsed_identifier>(*e_val_lhs_val0.record_fields[0]);
        EXPECT_EQ("x", e_val_lhs_val0_arg0.name);
        EXPECT_FALSE(e_val_lhs_val0_arg0.type);

        auto e_val_lhs_val0_val0 = std::get<parsed_int>(*e_val_lhs_val0.record_values[0]);
        EXPECT_EQ(1, e_val_lhs_val0_val0.value);

        auto unit = std::get<parsed_simple_unit>(*e_val_lhs_val0_val0.unit);
        EXPECT_EQ(unit_pref::m, unit.val.prefix);
        EXPECT_EQ(unit_sym::s, unit.val.symbol);

        auto e_val_lhs_arg1 = std::get<parsed_identifier>(*e_val_lhs.record_fields[1]);
        EXPECT_EQ("b", e_val_lhs_arg1.name);
        EXPECT_TRUE(e_val_lhs_arg1.type);

        auto type = std::get<parsed_quantity_type>(*e_val_lhs_arg1.type.value());
        EXPECT_EQ(quantity::voltage, type.type);

        auto e_val_lhs_val1 = std::get<parsed_int>(*e_val_lhs.record_values[1]);
        EXPECT_EQ(4, e_val_lhs_val1.value);

        auto unit1 = std::get<parsed_simple_unit>(*e_val_lhs_val1.unit);
        EXPECT_EQ(unit_pref::none, unit1.val.prefix);
        EXPECT_EQ(unit_sym::V, unit1.val.symbol);

        auto e_val_rhs = std::get<parsed_identifier>(*e_val.rhs);
        EXPECT_EQ("a", e_val_rhs.name);
        EXPECT_FALSE(e_val_rhs.type);

        auto e_body = std::get<parsed_identifier>(*e.body);
        EXPECT_EQ(src_location(1,49), e_body.loc);
        EXPECT_EQ("x", e_body.name);
        EXPECT_FALSE(e_body.type);
    }
    {
        std::string with = "with (let a = 1; {b = a + 3; c = 5 [Ohm];}); b*c";
        auto p = parser(with);
        auto e = std::get<parsed_with>(*p.parse_with());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_val = std::get<parsed_let>(*e.value);
        EXPECT_EQ(src_location(1,7), e_val.loc);

        auto e_val_id = std::get<parsed_identifier>(*e_val.identifier);
        EXPECT_EQ("a", e_val_id.name);
        EXPECT_FALSE(e_val_id.type);

        auto e_val_val = std::get<parsed_int>(*e_val.value);
        EXPECT_EQ(1, e_val_val.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_val_val.unit.get()));

        auto e_val_body = std::get<parsed_object>(*e_val.body);
        EXPECT_FALSE(e_val_body.record_name);
        EXPECT_EQ(2u, e_val_body.record_fields.size());
        EXPECT_EQ(2u, e_val_body.record_values.size());

        auto e_val_body_arg0 = std::get<parsed_identifier>(*e_val_body.record_fields[0]);
        EXPECT_EQ("b", e_val_body_arg0.name);
        EXPECT_FALSE(e_val_body_arg0.type);

        auto e_val_body_val0 = std::get<parsed_binary>(*e_val_body.record_values[0]);
        EXPECT_EQ(binary_op::add, e_val_body_val0.op);

        auto e_val_body_val0_lhs = std::get<parsed_identifier>(*e_val_body_val0.lhs);
        EXPECT_EQ("a", e_val_body_val0_lhs.name);
        EXPECT_FALSE(e_val_body_val0_lhs.type);

        auto e_val_body_val0_rhs = std::get<parsed_int>(*e_val_body_val0.rhs);
        EXPECT_EQ(3, e_val_body_val0_rhs.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_val_body_val0_rhs.unit.get()));

        auto e_val_body_arg1 = std::get<parsed_identifier>(*e_val_body.record_fields[1]);
        EXPECT_EQ("c", e_val_body_arg1.name);
        EXPECT_FALSE(e_val_body_arg1.type);

        auto e_val_body_val1 = std::get<parsed_int>(*e_val_body.record_values[1]);
        EXPECT_EQ(5, e_val_body_val1.value);

        auto unit = std::get<parsed_simple_unit>(*e_val_body_val1.unit);
        EXPECT_EQ(unit_pref::none, unit.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, unit.val.symbol);

        auto e_body = std::get<parsed_binary>(*e.body);
        EXPECT_EQ(src_location(1,47), e_body.loc);
        EXPECT_EQ(binary_op::mul, e_body.op);

        auto e_body_lhs = std::get<parsed_identifier>(*e_body.lhs);
        EXPECT_EQ("b", e_body_lhs.name);
        EXPECT_FALSE(e_body_lhs.type);

        auto e_body_rhs = std::get<parsed_identifier>(*e_body.rhs);
        EXPECT_EQ("c", e_body_rhs.name);
        EXPECT_FALSE(e_body_rhs.type);
    }
    {
        std::string with = "with a + 2; a";
        auto p = parser(with);
        auto e = std::get<parsed_with>(*p.parse_with());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_val = std::get<parsed_binary>(*e.value);
        EXPECT_EQ(src_location(1,8), e_val.loc);
        EXPECT_EQ(binary_op::add, e_val.op);

        auto e_val_lhs = std::get<parsed_identifier>(*e_val.lhs);
        EXPECT_EQ("a", e_val_lhs.name);
        EXPECT_FALSE(e_val_lhs.type);

        auto e_val_rhs = std::get<parsed_int>(*e_val.rhs);
        EXPECT_EQ(2, e_val_rhs.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_val_rhs.unit.get()));

        auto e_body = std::get<parsed_identifier>(*e.body);
        EXPECT_EQ(src_location(1,13), e_body.loc);
        EXPECT_EQ("a", e_body.name);
        EXPECT_FALSE(e_body.type);
    }
}

TEST(parser, conditional) {
    {
        std::string cond = "if a then 1 else 0";
        auto p = parser(cond);
        auto e = std::get<parsed_conditional>(*p.parse_conditional());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_cond = std::get<parsed_identifier>(*e.condition);
        EXPECT_EQ(src_location(1,4), e_cond.loc);
        EXPECT_EQ("a", e_cond.name);
        EXPECT_FALSE(e_cond.type);

        auto e_true = std::get<parsed_int>(*e.value_true);
        EXPECT_EQ(src_location(1,11), e_true.loc);
        EXPECT_EQ(1, e_true.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_true.unit.get()));

        auto e_false = std::get<parsed_int>(*e.value_false);
        EXPECT_EQ(src_location(1,18), e_false.loc);
        EXPECT_EQ(0, e_false.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_false.unit.get()));
    }
    {
        std::string cond = "if a&&b then x*y else x/y";
        auto p = parser(cond);
        auto e = std::get<parsed_conditional>(*p.parse_conditional());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_cond = std::get<parsed_binary>(*e.condition);
        EXPECT_EQ(src_location(1,5), e_cond.loc);
        EXPECT_EQ(binary_op::land, e_cond.op);

        auto e_cond_lhs = std::get<parsed_identifier>(*e_cond.lhs);
        EXPECT_EQ("a", e_cond_lhs.name);
        EXPECT_FALSE(e_cond_lhs.type);

        auto e_cond_rhs = std::get<parsed_identifier>(*e_cond.rhs);
        EXPECT_EQ("b", e_cond_rhs.name);
        EXPECT_FALSE(e_cond_rhs.type);

        auto e_true = std::get<parsed_binary>(*e.value_true);
        EXPECT_EQ(src_location(1,15), e_true.loc);
        EXPECT_EQ(binary_op::mul, e_true.op);

        auto e_true_lhs = std::get<parsed_identifier>(*e_true.lhs);
        EXPECT_EQ("x", e_true_lhs.name);
        EXPECT_FALSE(e_true_lhs.type);

        auto e_true_rhs = std::get<parsed_identifier>(*e_true.rhs);
        EXPECT_EQ("y", e_true_rhs.name);
        EXPECT_FALSE(e_true_rhs.type);

        auto e_false = std::get<parsed_binary>(*e.value_false);
        EXPECT_EQ(src_location(1,24), e_false.loc);
        EXPECT_EQ(binary_op::div, e_false.op);

        auto e_false_lhs = std::get<parsed_identifier>(*e_false.lhs);
        EXPECT_EQ("x", e_false_lhs.name);
        EXPECT_FALSE(e_false_lhs.type);

        auto e_false_rhs = std::get<parsed_identifier>(*e_false.rhs);
        EXPECT_EQ("y", e_false_rhs.name);
        EXPECT_FALSE(e_false_rhs.type);
    }
    {
        std::string cond = "if foo(a) then (let a = 2; a+5) else (let b = 0; 0)";
        auto p = parser(cond);
        auto e = std::get<parsed_conditional>(*p.parse_conditional());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_cond = std::get<parsed_call>(*e.condition);
        EXPECT_EQ(src_location(1,4), e_cond.loc);
        EXPECT_EQ("foo", e_cond.function_name);
        EXPECT_EQ(1u, e_cond.call_args.size());

        auto e_cond_arg = std::get<parsed_identifier>(*e_cond.call_args[0]);
        EXPECT_EQ("a", e_cond_arg.name);
        EXPECT_FALSE(e_cond_arg.type);

        auto e_true = std::get<parsed_let>(*e.value_true);
        EXPECT_EQ(src_location(1,17), e_true.loc);

        auto e_true_id = std::get<parsed_identifier>(*e_true.identifier);
        EXPECT_EQ("a", e_true_id.name);
        EXPECT_FALSE(e_true_id.type);

        auto e_true_val = std::get<parsed_int>(*e_true.value);
        EXPECT_EQ(2, e_true_val.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_true_val.unit.get()));

        auto e_true_body = std::get<parsed_binary>(*e_true.body);
        EXPECT_EQ(binary_op::add, e_true_body.op);

        auto e_true_body_lhs = std::get<parsed_identifier>(*e_true_body.lhs);
        EXPECT_EQ("a", e_true_body_lhs.name);
        EXPECT_FALSE(e_true_body_lhs.type);

        auto e_true_body_rhs = std::get<parsed_int>(*e_true_body.rhs);
        EXPECT_EQ(5, e_true_body_rhs.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_true_body_rhs.unit.get()));

        auto e_false = std::get<parsed_let>(*e.value_false);
        EXPECT_EQ(src_location(1,39), e_false.loc);

        auto e_false_id = std::get<parsed_identifier>(*e_false.identifier);
        EXPECT_EQ("b", e_false_id.name);
        EXPECT_FALSE(e_false_id.type);

        auto e_false_val = std::get<parsed_int>(*e_false.value);
        EXPECT_EQ(0, e_false_val.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_false_val.unit.get()));

        auto e_false_body = std::get<parsed_int>(*e_false.body);
        EXPECT_EQ(0, e_false_body.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_false_body.unit.get()));
    }
    {
        std::string cond = "if X.y then 0 else 1";
        auto p = parser(cond);
        auto e = std::get<parsed_conditional>(*p.parse_conditional());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_cond = std::get<parsed_binary>(*e.condition);
        EXPECT_EQ(binary_op::dot, e_cond.op);

        auto e_cond_lhs = std::get<parsed_identifier>(*e_cond.lhs);
        EXPECT_EQ("X", e_cond_lhs.name);
        EXPECT_FALSE(e_cond_lhs.type);

        auto e_cond_rhs = std::get<parsed_identifier>(*e_cond.rhs);
        EXPECT_EQ("y", e_cond_rhs.name);
        EXPECT_FALSE(e_cond_rhs.type);

        auto e_true = std::get<parsed_int>(*e.value_true);
        EXPECT_EQ(0, e_true.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_true.unit.get()));

        auto e_false = std::get<parsed_int>(*e.value_false);
        EXPECT_EQ(1, e_false.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_false.unit.get()));
    }
}

// exp, exprelr, log, cos, sin, abs, !, -, +, max, min
TEST(parser, prefix_expr) {
    {
        std::string val = "exp(-3)";
        auto p = parser(val);
        auto e = std::get<parsed_unary>(*p.parse_prefix_expr());
        EXPECT_EQ(unary_op::exp, e.op);
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_arg = std::get<parsed_unary>(*e.value);
        EXPECT_EQ(src_location(1,5), e_arg.loc);
        EXPECT_EQ(unary_op::neg, e_arg.op);

        auto e_arg_arg = std::get<parsed_int>(*e_arg.value);
        EXPECT_EQ(src_location(1,6), e_arg_arg.loc);
        EXPECT_EQ(3, e_arg_arg.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_arg_arg.unit.get()));
    }
    {
        std::string val = "exprelr(-exp(3))";
        auto p = parser(val);
        auto e = std::get<parsed_unary>(*p.parse_prefix_expr());
        EXPECT_EQ(unary_op::exprelr, e.op);
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_arg = std::get<parsed_unary>(*e.value);
        EXPECT_EQ(src_location(1,9), e_arg.loc);
        EXPECT_EQ(unary_op::neg, e_arg.op);

        auto e_arg_arg = std::get<parsed_unary>(*e_arg.value);
        EXPECT_EQ(src_location(1,10), e_arg_arg.loc);
        EXPECT_EQ(unary_op::exp, e_arg_arg.op);

        auto e_arg_arg_arg = std::get<parsed_int>(*e_arg_arg.value);
        EXPECT_EQ(src_location(1,14), e_arg_arg_arg.loc);
        EXPECT_EQ(3, e_arg_arg_arg.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_arg_arg_arg.unit.get()));
    }
    {
        std::string val = "log(foo(+3.2e3))";
        auto p = parser(val);
        auto e = std::get<parsed_unary>(*p.parse_prefix_expr());
        EXPECT_EQ(unary_op::log, e.op);
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_arg = std::get<parsed_call>(*e.value);
        EXPECT_EQ(src_location(1,5), e_arg.loc);
        EXPECT_EQ("foo", e_arg.function_name);
        EXPECT_EQ(1, e_arg.call_args.size());

        auto e_arg_arg = std::get<parsed_float>(*e_arg.call_args[0]);
        EXPECT_EQ(src_location(1,10), e_arg_arg.loc);
        EXPECT_EQ(3.2e3, e_arg_arg.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_arg_arg.unit.get()));
    }
    {
        std::string val = "cos({a = -3.14159;}.a)";
        auto p = parser(val);
        auto e = std::get<parsed_unary>(*p.parse_prefix_expr());
        EXPECT_EQ(unary_op::cos, e.op);
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_arg = std::get<parsed_binary>(*e.value);
        EXPECT_EQ(src_location(1,20), e_arg.loc);
        EXPECT_EQ(binary_op::dot, e_arg.op);

        auto e_arg_lhs = std::get<parsed_object>(*e_arg.lhs);
        EXPECT_EQ(src_location(1,5), e_arg_lhs.loc);
        EXPECT_FALSE(e_arg_lhs.record_name);
        EXPECT_EQ(1u, e_arg_lhs.record_fields.size());
        EXPECT_EQ(1u, e_arg_lhs.record_values.size());

        auto e_arg_lhs_arg = std::get<parsed_identifier>(*e_arg_lhs.record_fields[0]);
        EXPECT_EQ("a", e_arg_lhs_arg.name);
        EXPECT_FALSE(e_arg_lhs_arg.type);

        auto e_arg_lhs_val = std::get<parsed_unary>(*e_arg_lhs.record_values[0]);
        EXPECT_EQ(unary_op::neg, e_arg_lhs_val.op);

        auto e_arg_lhs_val_val = std::get<parsed_float>(*e_arg_lhs_val.value);
        EXPECT_EQ(3.14159, e_arg_lhs_val_val.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_arg_lhs_val_val.unit.get()));

        auto e_arg_rhs = std::get<parsed_identifier>(*e_arg.rhs);
        EXPECT_EQ("a", e_arg_rhs.name);
        EXPECT_FALSE(e_arg_rhs.type);
    }
    {
        std::string val = "let a = 2; sin(a)";
        auto p = parser(val);
        auto e = std::get<parsed_let>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_id = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ(src_location(1,5), e_id.loc);
        EXPECT_EQ("a", e_id.name);
        EXPECT_FALSE(e_id.type);

        auto e_val = std::get<parsed_int>(*e.value);
        EXPECT_EQ(src_location(1,9), e_val.loc);
        EXPECT_EQ(2, e_val.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_val.unit.get()));

        auto e_body = std::get<parsed_unary>(*e.body);
        EXPECT_EQ(src_location(1,12), e_body.loc);
        EXPECT_EQ(unary_op::sin, e_body.op);

        auto e_body_val = std::get<parsed_identifier>(*e_body.value);
        EXPECT_EQ(src_location(1,16), e_body_val.loc);
        EXPECT_EQ("a", e_body_val.name);
        EXPECT_FALSE(e_body_val.type);
    }
    {
        std::string val = "abs(if a>3 then a else b)";
        auto p = parser(val);
        auto e = std::get<parsed_unary>(*p.parse_prefix_expr());
        EXPECT_EQ(src_location(1,1), e.loc);
        EXPECT_EQ(unary_op::abs, e.op);

        auto e_val = std::get<parsed_conditional>(*e.value);
        EXPECT_EQ(src_location(1,5), e_val.loc);

        auto e_val_cond = std::get<parsed_binary>(*e_val.condition);
        EXPECT_EQ(binary_op::gt, e_val_cond.op);

        auto e_val_cond_lhs = std::get<parsed_identifier>(*e_val_cond.lhs);
        EXPECT_EQ("a", e_val_cond_lhs.name);
        EXPECT_FALSE(e_val_cond_lhs.type);

        auto e_val_cond_rhs = std::get<parsed_int>(*e_val_cond.rhs);
        EXPECT_EQ(3, e_val_cond_rhs.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(e_val_cond_rhs.unit.get()));

        auto e_val_true = std::get<parsed_identifier>(*e_val.value_true);
        EXPECT_EQ("a", e_val_true.name);
        EXPECT_FALSE(e_val_true.type);

        auto e_val_false = std::get<parsed_identifier>(*e_val.value_false);
        EXPECT_EQ("b", e_val_false.name);
        EXPECT_FALSE(e_val_false.type);
    }
    {
        std::string val = "max(+3 [mV], min(a, b))";
        auto p = parser(val);
        auto e = std::get<parsed_binary>(*p.parse_prefix_expr());
        EXPECT_EQ(src_location(1,1), e.loc);
        EXPECT_EQ(binary_op::max, e.op);

        auto e_lhs = std::get<parsed_int>(*e.lhs);
        EXPECT_EQ(3, e_lhs.value);

        auto unit = std::get<parsed_simple_unit>(*e_lhs.unit);
        EXPECT_EQ(unit_pref::m, unit.val.prefix);
        EXPECT_EQ(unit_sym::V, unit.val.symbol);

        auto e_rhs = std::get<parsed_binary>(*e.rhs);
        EXPECT_EQ(binary_op::min, e_rhs.op);

        auto e_rhs_lhs = std::get<parsed_identifier>(*e_rhs.lhs);
        EXPECT_EQ("a", e_rhs_lhs.name);
        EXPECT_FALSE(e_rhs_lhs.type);

        auto e_rhs_rhs = std::get<parsed_identifier>(*e_rhs.rhs);
        EXPECT_EQ("b", e_rhs_rhs.name);
        EXPECT_FALSE(e_rhs_rhs.type);
    }
}

namespace {
double eval(p_expr e) {
    if (auto v = std::get_if<parsed_int>(e.get())) {
        return v->value;
    }
    if (auto v = std::get_if<parsed_float>(e.get())) {
        return v->value;
    }
    if (auto v = std::get_if<parsed_binary>(e.get())) {
        auto lhs = eval(v->lhs);
        auto rhs = eval(v->rhs);
        switch (v->op) {
            case binary_op::add:  return lhs + rhs;
            case binary_op::sub:  return lhs - rhs;
            case binary_op::mul:  return lhs * rhs;
            case binary_op::div:  return lhs / rhs;
            case binary_op::lt:   return lhs < rhs;
            case binary_op::le:   return lhs <= rhs;
            case binary_op::gt:   return lhs > rhs;
            case binary_op::ge:   return lhs >= rhs;
            case binary_op::ne:   return lhs != rhs;
            case binary_op::eq:   return lhs == rhs;
            case binary_op::land: return lhs && rhs;
            case binary_op::lor:  return lhs || rhs;
            case binary_op::pow:  return std::pow(lhs, rhs);
            case binary_op::min:  return std::min(lhs, rhs);
            case binary_op::max:  return std::max(lhs, rhs);
            default:;
        }
    }
    if (auto v = std::get_if<parsed_unary>(e.get())) {
        auto val = eval(v->value);
        switch (v->op) {
            case unary_op::neg: return -val;
            default:;
        }
    }
    return std::numeric_limits<double>::quiet_NaN();
}
}
TEST(parser, infix_expr) {
    using std::pow;

    std::pair<std::string, double> tests[] = {
        // simple
        {"2+3",      2. + 3.},
        {"2-3",      2. - 3.},
        {"2*3",      2. * 3.},
        {"2/3",      2. / 3.},
        {"2^3",      pow(2., 3.)},
        {"min(2,3)", 2.},
        {"min(3,2)", 2.},
        {"max(2,3)", 3.},
        {"max(3,2)", 3.},

        // more complicated
        {"2+3*4", 2. + (3 * 4)},
        {"2*3-5", (2. * 3) - 5.},
        {"2+3*(-2)", 2. + (3 * -2)},
        {"2+3*(-+2)", 2. + (3 * -+2)},
        {"2/3*4", (2. / 3.) * 4.},
        {"min(2+3, 4/2)", 4. / 2},
        {"max(2+3, 4/2)", 2. + 3.},
        {"max(2+3, min(12, 24))", 12.},
        {"max(min(12, 24), 2+3)", 12.},
        {"2 * 7 - 3 * 11 + 4 * 13", 2. * 7. - 3. * 11. + 4. * 13.},

        // right associative
        {"2^3^1.5", pow(2., pow(3., 1.5))},
        {"2^3^1.5^2", pow(2., pow(3., pow(1.5, 2.)))},
        {"2^2^3", pow(2., pow(2., 3.))},
        {"(2^2)^3", pow(pow(2., 2.), 3.)},
        {"3./2^7.", 3. / pow(2., 7.)},
        {"3^2*5.", pow(3., 2.) * 5.},

        // multilevel
        {"1-2*3^4*5^2^3-3^2^3/4/8-5", 1. - 2 * pow(3., 4.) * pow(5., pow(2., 3.)) - pow(3, pow(2., 3.)) / 4. / 8. - 5}
    };

    for (const auto& test_case: tests) {
        auto p = parser(test_case.first);
        auto e = p.parse_expr();
        EXPECT_NEAR(eval(e), test_case.second, 1e-10);
    }

    std::pair<std::string, bool> bool_tests[] = {
        {"0 && 0 || 1", true},
        {"(0 && 0) || 1", true},
        {"0 && (0 || 1)", false},
        {"3<2 && 1 || 4>1", true},
        {"(3<2 && 1) || 4>1", true},
        {"3<2 && (1 || 4>1)", false},
        {"(3<2) && (1 || (4>1))", false},
    };

    for (const auto& test_case: bool_tests) {
        auto p = parser(test_case.first);
        auto e = p.parse_expr();
        EXPECT_NEAR(eval(e), test_case.second, 1e-10);
    }
}

TEST(parser, function) {
    {
        std::string fun = "function foo(){0};";
        auto p = parser(fun);
        auto e = std::get<parsed_function>(*p.parse_function());
        EXPECT_EQ("foo", e.name);
        EXPECT_EQ(0u, e.args.size());

        auto body = std::get<parsed_int>(*e.body);
        EXPECT_EQ(0, body.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(body.unit.get()));

        EXPECT_FALSE(e.ret);
    }
    {
        std::string fun = "function foo(a: voltage): voltage {a;};";
        auto p = parser(fun);
        auto e = std::get<parsed_function>(*p.parse_function());
        EXPECT_EQ("foo", e.name);
        EXPECT_EQ(1u, e.args.size());

        auto arg0 = std::get<parsed_identifier>(*e.args.front());
        EXPECT_EQ("a", arg0.name);
        EXPECT_TRUE(arg0.type);

        auto arg0_type = std::get<parsed_quantity_type>(*arg0.type.value());
        EXPECT_EQ(quantity::voltage, arg0_type.type);

        auto body = std::get<parsed_identifier>(*e.body);
        EXPECT_EQ("a", body.name);
        EXPECT_FALSE(body.type);

        EXPECT_TRUE(e.ret);
        auto ret_type = std::get<parsed_quantity_type>(*e.ret.value());
        EXPECT_EQ(quantity::voltage, ret_type.type);
    }
    {
        std::string fun =
            "function bar(a:resistance/time, b:rec0, c:{a: voltage}) {\n"
            "  let x = c.a + b.a * a;\n"
            "  x;\n"
            "};";
        auto p = parser(fun);
        auto e = std::get<parsed_function>(*p.parse_function());
        EXPECT_EQ("bar", e.name);
        EXPECT_EQ(3u, e.args.size());

        auto arg0 = std::get<parsed_identifier>(*e.args[0]);
        EXPECT_EQ("a", arg0.name);
        EXPECT_TRUE(arg0.type);

        auto arg0_type = std::get<parsed_binary_quantity_type>(*arg0.type.value());
        EXPECT_EQ(t_binary_op::div, arg0_type.op);

        auto arg0_lhs = std::get<parsed_quantity_type>(*arg0_type.lhs);
        auto arg0_rhs = std::get<parsed_quantity_type>(*arg0_type.rhs);
        EXPECT_EQ(quantity::resistance, arg0_lhs.type);
        EXPECT_EQ(quantity::time, arg0_rhs.type);

        auto arg1 = std::get<parsed_identifier>(*e.args[1]);
        EXPECT_EQ("b", arg1.name);
        EXPECT_TRUE(arg1.type);

        auto arg1_type = std::get<parsed_record_alias_type>(*arg1.type.value());
        EXPECT_EQ("rec0", arg1_type.name);

        auto arg2 = std::get<parsed_identifier>(*e.args[2]);
        EXPECT_EQ("c", arg2.name);
        EXPECT_TRUE(arg2.type);

        auto arg2_type = std::get<parsed_record_type>(*arg2.type.value());
        EXPECT_EQ(1u, arg2_type.fields.size());

        EXPECT_EQ("a", arg2_type.fields[0].first);
        auto arg2_arg0 = std::get<parsed_quantity_type>(*arg2_type.fields[0].second);
        EXPECT_EQ(quantity::voltage, arg2_arg0.type);

        auto body = std::get<parsed_let>(*e.body);
        auto body_iden = std::get<parsed_identifier>(*body.identifier);
        EXPECT_EQ("x", body_iden.name);
        EXPECT_FALSE(body_iden.type);

        auto body_val  = std::get<parsed_binary>(*body.value);
        EXPECT_EQ(binary_op::add, body_val.op);

        auto val_lhs = std::get<parsed_binary>(*body_val.lhs);
        EXPECT_EQ(binary_op::dot, val_lhs.op);

        auto val_lhs_lhs = std::get<parsed_identifier>(*val_lhs.lhs);
        EXPECT_EQ("c", val_lhs_lhs.name);
        EXPECT_FALSE(val_lhs_lhs.type);

        auto val_lhs_rhs = std::get<parsed_identifier>(*val_lhs.rhs);
        EXPECT_EQ("a", val_lhs_rhs.name);
        EXPECT_FALSE(val_lhs_rhs.type);

        auto val_rhs = std::get<parsed_binary>(*body_val.rhs);
        EXPECT_EQ(binary_op::mul, val_rhs.op);

        auto val_rhs_lhs = std::get<parsed_binary>(*val_rhs.lhs);
        EXPECT_EQ(binary_op::dot, val_rhs_lhs.op);

        auto val_rhs_lhs_lhs = std::get<parsed_identifier>(*val_rhs_lhs.lhs);
        EXPECT_EQ("b", val_rhs_lhs_lhs.name);
        EXPECT_FALSE(val_rhs_lhs_lhs.type);

        auto val_rhs_lhs_rhs = std::get<parsed_identifier>(*val_rhs_lhs.rhs);
        EXPECT_EQ("a", val_rhs_lhs_rhs.name);
        EXPECT_FALSE(val_rhs_lhs_rhs.type);

        auto val_rhs_rhs = std::get<parsed_identifier>(*val_rhs.rhs);
        EXPECT_EQ("a", val_rhs_rhs.name);
        EXPECT_FALSE(val_rhs_rhs.type);

        auto body_body = std::get<parsed_identifier>(*body.body);
        EXPECT_EQ("x", body_body.name);
        EXPECT_FALSE(body_body.type);

        EXPECT_FALSE(e.ret);
    }
    {
        std::vector<std::string> invalid_functions = {
            "function foo{};",
            "function foo(){};",
            "function foo(a, b){};",
        };
        for (const auto& test_case: invalid_functions) {
            auto p = parser(test_case);
            EXPECT_THROW(p.parse_function(), std::runtime_error);
        }
    }
}

TEST(parser, record) {
    {
        std::string rec = "record rec {};";
        auto p = parser(rec);
        auto e = std::get<parsed_record_alias>(*p.parse_record_alias());
        EXPECT_EQ("rec", e.name);

        auto body = std::get<parsed_record_type>(*e.type);
        EXPECT_EQ(0u, body.fields.size());
    }
    {
        std::string rec = "record rec {a: voltage, b: time};";
        auto p = parser(rec);
        auto e = std::get<parsed_record_alias>(*p.parse_record_alias());
        EXPECT_EQ("rec", e.name);

        auto body = std::get<parsed_record_type>(*e.type);
        EXPECT_EQ(2u, body.fields.size());
        EXPECT_EQ("a", body.fields[0].first);
        EXPECT_EQ("b", body.fields[1].first);

        auto field0 = std::get<parsed_quantity_type>(*body.fields[0].second);
        EXPECT_EQ(quantity::voltage, field0.type);

        auto field1 = std::get<parsed_quantity_type>(*body.fields[1].second);
        EXPECT_EQ(quantity::time, field1.type);
    }
    {
        std::vector<std::string> invalid_functions = {
                "record foo{};",
                "record foo{a};",
                "record foo(a: voltage)",
                "record foo{a: voltage; b:voltage};",
                "record foo{time};",
        };
        for (const auto& test_case: invalid_functions) {
            auto p = parser(test_case);
            EXPECT_THROW(p.parse_function(), std::runtime_error);
        }
    }
}

// including "density" keyword
TEST(parser, parameter) {
    {
        std::string param = "parameter a = 3 [mV];";
        auto p = parser(param);
        auto e = std::get<parsed_parameter>(*p.parse_parameter());

        auto id  = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ("a", id.name);
        EXPECT_FALSE(id.type);

        auto val = std::get<parsed_int>(*e.value);
        EXPECT_EQ(3, val.value);

        auto unit = std::get<parsed_simple_unit>(*val.unit);
        EXPECT_EQ(unit_pref::m, unit.val.prefix);
        EXPECT_EQ(unit_sym::V, unit.val.symbol);
    }
    {
        std::string param = "parameter iconc: concentration = get_conc(x);";
        auto p = parser(param);
        auto e = std::get<parsed_parameter>(*p.parse_parameter());

        auto id  = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ("iconc", id.name);
        EXPECT_TRUE(id.type);

        auto type = std::get<parsed_quantity_type>(*id.type.value());
        EXPECT_EQ(quantity::concentration, type.type);

        auto val = std::get<parsed_call>(*e.value);
        EXPECT_EQ("get_conc", val.function_name);
        EXPECT_EQ(1u, val.call_args.size());

        auto arg = std::get<parsed_identifier>(*val.call_args[0]);
        EXPECT_EQ("x", arg.name);
        EXPECT_FALSE(arg.type);
    }
}

TEST(parser, constant) {
    {
        std::string param = "constant my_conc = (let a = 2; a*foo(a));";
        auto p = parser(param);
        auto e = std::get<parsed_constant>(*p.parse_constant());

        auto id  = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ("my_conc", id.name);
        EXPECT_FALSE(id.type);

        auto val = std::get<parsed_let>(*e.value);
        auto v_id = std::get<parsed_identifier>(*val.identifier);
        EXPECT_EQ("a", v_id.name);
        EXPECT_FALSE(v_id.type);

        auto v_val = std::get<parsed_int>(*val.value);
        EXPECT_EQ(2, v_val.value);
        EXPECT_TRUE(std::get_if<parsed_no_unit>(v_val.unit.get()));

        auto v_body = std::get<parsed_binary>(*val.body);
        EXPECT_EQ(binary_op::mul, v_body.op);

        auto lhs = std::get<parsed_identifier>(*v_body.lhs);
        EXPECT_EQ("a", lhs.name);
        EXPECT_FALSE(lhs.type);

        auto rhs = std::get<parsed_call>(*v_body.rhs);
        EXPECT_EQ("foo", rhs.function_name);
        EXPECT_EQ(1u, rhs.call_args.size());

        auto arg = std::get<parsed_identifier>(*rhs.call_args[0]);
        EXPECT_EQ("a", arg.name);
        EXPECT_FALSE(arg.type);
    }
}

TEST(parser, state) {
    {
        std::string state = "state s: concentration/volume;";
        auto p = parser(state);
        auto e = std::get<parsed_state>(*p.parse_state());

        auto id  = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ("s", id.name);
        EXPECT_TRUE(id.type);

        auto type = std::get<parsed_binary_quantity_type>(*id.type.value());
        EXPECT_EQ(t_binary_op::div, type.op);

        auto lhs = std::get<parsed_quantity_type>(*type.lhs);
        EXPECT_EQ(quantity::concentration, lhs.type);

        auto rhs = std::get<parsed_quantity_type>(*type.rhs);
        EXPECT_EQ(quantity::volume, rhs.type);
    }
    {
        std::string state = "state gbar;";
        auto p = parser(state);
        auto e = std::get<parsed_state>(*p.parse_state());

        auto id  = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ("gbar", id.name);
        EXPECT_FALSE(id.type);
    }
}

TEST(parser, bind) {
    {
        std::string binding = "bind a: voltage = membrane_potential;";
        auto p = parser(binding);
        auto e = std::get<parsed_bind>(*p.parse_binding());

        auto id  = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ("a", id.name);
        EXPECT_TRUE(id.type);

        auto type = std::get<parsed_quantity_type>(*id.type.value());
        EXPECT_EQ(quantity::voltage, type.type);

        EXPECT_EQ(bindable::membrane_potential, e.bind);
        EXPECT_FALSE(e.ion);
    }
    {
        std::string binding = "bind ica = internal_concentration(\"ca\");";
        auto p = parser(binding);
        auto e = std::get<parsed_bind>(*p.parse_binding());

        auto id  = std::get<parsed_identifier>(*e.identifier);
        EXPECT_EQ("ica", id.name);
        EXPECT_FALSE(id.type);

        EXPECT_EQ(bindable::internal_concentration, e.bind);
        EXPECT_TRUE(e.ion);
        EXPECT_EQ("ca", e.ion.value());
    }
}

TEST(parser, initial) {
    std::string init = "initial st = 0 [S];";
    auto p = parser(init);
    auto e = std::get<parsed_initial>(*p.parse_initial());

    auto id = std::get<parsed_identifier>(*e.identifier);
    EXPECT_EQ("st", id.name);
    EXPECT_FALSE(id.type);

    auto val = std::get<parsed_int>(*e.value);
    EXPECT_EQ(0, val.value);

    auto unit = std::get<parsed_simple_unit>(*val.unit);
    EXPECT_EQ(unit_pref::none, unit.val.prefix);
    EXPECT_EQ(unit_sym::S, unit.val.symbol);
}

TEST(parser, evolve) {
    std::string evolve = "evolve st':conductance/time = solve_kin().s';";
    auto p = parser(evolve);
    auto e = std::get<parsed_evolve>(*p.parse_evolve());

    auto id = std::get<parsed_identifier>(*e.identifier);
    EXPECT_EQ("st'", id.name);
    EXPECT_TRUE(id.type);

    auto type = std::get<parsed_binary_quantity_type>(*id.type.value());
    EXPECT_EQ(t_binary_op::div, type.op);

    auto t_lhs = std::get<parsed_quantity_type>(*type.lhs);
    EXPECT_EQ(quantity::conductance, t_lhs.type);

    auto t_rhs = std::get<parsed_quantity_type>(*type.rhs);
    EXPECT_EQ(quantity::time, t_rhs.type);

    auto val = std::get<parsed_binary>(*e.value);
    EXPECT_EQ(binary_op::dot, val.op);

    auto v_lhs = std::get<parsed_call>(*val.lhs);
    EXPECT_EQ("solve_kin", v_lhs.function_name);
    EXPECT_TRUE(v_lhs.call_args.empty());

    auto v_rhs = std::get<parsed_identifier>(*val.rhs);
    EXPECT_EQ("s'", v_rhs.name);
    EXPECT_FALSE(v_rhs.type);
}

TEST(parser, effect) {
    std::string effect = "effect current_density(\"k\") = gbar*s*(v - ek);";
    auto p = parser(effect);
    auto e = std::get<parsed_effect>(*p.parse_effect());

    EXPECT_EQ(affectable::current_density, e.effect);
    EXPECT_EQ("k", e.ion);
    EXPECT_FALSE(e.type);

    auto val = std::get<parsed_binary>(*e.value);
    EXPECT_EQ(binary_op::mul, val.op);

    auto lhs = std::get<parsed_binary>(*val.lhs);
    EXPECT_EQ(binary_op::mul, lhs.op);

    auto l_lhs = std::get<parsed_identifier>(*lhs.lhs);
    EXPECT_EQ("gbar", l_lhs.name);
    EXPECT_FALSE(l_lhs.type);

    auto l_rhs = std::get<parsed_identifier>(*lhs.rhs);
    EXPECT_EQ("s", l_rhs.name);
    EXPECT_FALSE(l_rhs.type);

    auto rhs = std::get<parsed_binary>(*val.rhs);
    EXPECT_EQ(binary_op::sub, rhs.op);

    auto r_lhs = std::get<parsed_identifier>(*rhs.lhs);
    EXPECT_EQ("v", r_lhs.name);
    EXPECT_FALSE(r_lhs.type);

    auto r_rhs = std::get<parsed_identifier>(*rhs.rhs);
    EXPECT_EQ("ek", r_rhs.name);
    EXPECT_FALSE(r_rhs.type);
}

TEST(parser, export) {
    std::string exp = "export gbar;";
    auto p = parser(exp);
    auto e = std::get<parsed_export>(*p.parse_export());

    auto id = std::get<parsed_identifier>(*e.identifier);
    EXPECT_EQ("gbar", id.name);
    EXPECT_FALSE(id.type);
}

TEST(parser, mechanism) {
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
            "    effect molar_flow_rate(\"ca\") = -gamma*flux - (cai - minCai)/decay;\n"
            "}";
        auto p = parser(mech);
        EXPECT_NO_THROW(p.parse_mechanism());
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
            "       let w_plastic = S.w_plastic + S.apost;\n"
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
            "    };\n"
            "\n"
            "    effect current = expsyn.g*(v - e);\n"
            "}";
        auto p = parser(mech);
        EXPECT_NO_THROW(p.parse_mechanism());
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
        EXPECT_NO_THROW(p.parse_mechanism());
    }
}