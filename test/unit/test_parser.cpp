#include <string>
#include <variant>
#include <vector>

#include <arblang/token.hpp>
#include <arblang/parser.hpp>

#include "../gtest.h"

using namespace al;
using namespace raw_ir;

// TODO test exceptions properly

TEST(parser, unit) {
    using namespace u_raw_ir;
    {
        std::string unit = "mV";
        auto p = parser(unit);
        auto u = std::get<simple_unit>(*p.try_parse_unit().value());
        EXPECT_EQ("mV", u.spelling);
        EXPECT_EQ(unit_pref::m, u.val.prefix);
        EXPECT_EQ(unit_sym::V, u.val.symbol);
    }
    {
        std::string unit = "mmol/kA";
        auto p = parser(unit);
        auto u = std::get<binary_unit>(*p.try_parse_unit().value());
        auto lhs = std::get<simple_unit>(*u.lhs);
        auto rhs = std::get<simple_unit>(*u.rhs);

        EXPECT_EQ("mmol", lhs.spelling);
        EXPECT_EQ(unit_pref::m, lhs.val.prefix);
        EXPECT_EQ(unit_sym::mol, lhs.val.symbol);

        EXPECT_EQ("kA", rhs.spelling);
        EXPECT_EQ(unit_pref::k, rhs.val.prefix);
        EXPECT_EQ(unit_sym::A, rhs.val.symbol);

        EXPECT_EQ(u_binary_op::div, u.op);
    }
    {
        std::string unit = "K^-2";
        auto p = parser(unit);
        auto u = std::get<binary_unit>(*p.try_parse_unit().value());
        auto lhs = std::get<simple_unit>(*u.lhs);
        auto rhs = std::get<integer_unit>(*u.rhs);

        EXPECT_EQ("K", lhs.spelling);
        EXPECT_EQ(unit_pref::none, lhs.val.prefix);
        EXPECT_EQ(unit_sym::K, lhs.val.symbol);

        EXPECT_EQ(-2, rhs.val);

        EXPECT_EQ(u_binary_op::pow, u.op);
    }
    {
        std::string unit = "Ohm*uV/YS";
        auto p = parser(unit);
        auto u = std::get<binary_unit>(*p.try_parse_unit().value());

        EXPECT_EQ(u_binary_op::div, u.op);
        auto lhs_0 = std::get<binary_unit>(*u.lhs); // Ohm*uV
        auto rhs_0 = std::get<simple_unit>(*u.rhs); // YS

        EXPECT_EQ(u_binary_op::mul, lhs_0.op);
        auto lhs_1 = std::get<simple_unit>(*lhs_0.lhs); // Ohm
        auto rhs_1 = std::get<simple_unit>(*lhs_0.rhs); // uV

        EXPECT_EQ("Ohm", lhs_1.spelling);
        EXPECT_EQ(unit_pref::none, lhs_1.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_1.val.symbol);

        EXPECT_EQ("uV", rhs_1.spelling);
        EXPECT_EQ(unit_pref::u, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::V, rhs_1.val.symbol);

        EXPECT_EQ("YS", rhs_0.spelling);
        EXPECT_EQ(unit_pref::Y, rhs_0.val.prefix);
        EXPECT_EQ(unit_sym::S, rhs_0.val.symbol);
    }
    {
        std::string unit = "Ohm^2/daC/K^-3";
        auto p = parser(unit);
        auto u = std::get<binary_unit>(*p.try_parse_unit().value());

        EXPECT_EQ(u_binary_op::div, u.op);
        auto lhs_0 = std::get<binary_unit>(*u.lhs); // Ohm^2/daC
        auto rhs_0 = std::get<binary_unit>(*u.rhs); // K^-3

        EXPECT_EQ(u_binary_op::div, lhs_0.op);
        auto lhs_1 = std::get<binary_unit>(*lhs_0.lhs); // Ohm^2
        auto rhs_1 = std::get<simple_unit>(*lhs_0.rhs); // daC

        EXPECT_EQ(u_binary_op::pow, lhs_1.op);
        auto lhs_2 = std::get<simple_unit>(*lhs_1.lhs);  // Ohm
        auto rhs_2 = std::get<integer_unit>(*lhs_1.rhs); // 2

        EXPECT_EQ(u_binary_op::pow, rhs_0.op);
        auto lhs_3 = std::get<simple_unit>(*rhs_0.lhs);  // K
        auto rhs_3 = std::get<integer_unit>(*rhs_0.rhs); // -3

        EXPECT_EQ("daC", rhs_1.spelling);
        EXPECT_EQ(unit_pref::da, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::C, rhs_1.val.symbol);

        EXPECT_EQ("Ohm", lhs_2.spelling);
        EXPECT_EQ(unit_pref::none, lhs_2.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_2.val.symbol);

        EXPECT_EQ(2, rhs_2.val);

        EXPECT_EQ("K", lhs_3.spelling);
        EXPECT_EQ(unit_pref::none, lhs_3.val.prefix);
        EXPECT_EQ(unit_sym::K, lhs_3.val.symbol);

        EXPECT_EQ(-3, rhs_3.val);
    }
    {
        std::string unit = "Ohm^2/daC*mK^-1";
        auto p = parser(unit);
        auto u = std::get<binary_unit>(*p.try_parse_unit().value());

        EXPECT_EQ(u_binary_op::mul, u.op);
        auto lhs_0 = std::get<binary_unit>(*u.lhs); // Ohm^2/daC
        auto rhs_0 = std::get<binary_unit>(*u.rhs); // K^-3

        EXPECT_EQ(u_binary_op::div, lhs_0.op);
        auto lhs_1 = std::get<binary_unit>(*lhs_0.lhs); // Ohm^2
        auto rhs_1 = std::get<simple_unit>(*lhs_0.rhs); // daC

        EXPECT_EQ(u_binary_op::pow, lhs_1.op);
        auto lhs_2 = std::get<simple_unit>(*lhs_1.lhs);  // Ohm
        auto rhs_2 = std::get<integer_unit>(*lhs_1.rhs); // 2

        EXPECT_EQ(u_binary_op::pow, rhs_0.op);
        auto lhs_3 = std::get<simple_unit>(*rhs_0.lhs);  // K
        auto rhs_3 = std::get<integer_unit>(*rhs_0.rhs); // -3

        EXPECT_EQ("daC", rhs_1.spelling);
        EXPECT_EQ(unit_pref::da, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::C, rhs_1.val.symbol);

        EXPECT_EQ("Ohm", lhs_2.spelling);
        EXPECT_EQ(unit_pref::none, lhs_2.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_2.val.symbol);

        EXPECT_EQ(2, rhs_2.val);

        EXPECT_EQ("mK", lhs_3.spelling);
        EXPECT_EQ(unit_pref::m, lhs_3.val.prefix);
        EXPECT_EQ(unit_sym::K, lhs_3.val.symbol);

        EXPECT_EQ(-1, rhs_3.val);
    }
    {
        std::string unit = "(Ohm/V)*A";
        auto p = parser(unit);
        auto u = std::get<binary_unit>(*p.try_parse_unit().value());

        EXPECT_EQ(u_binary_op::mul, u.op);
        auto lhs_0 = std::get<binary_unit>(*u.lhs); // Ohm/V
        auto rhs_0 = std::get<simple_unit>(*u.rhs); // A

        EXPECT_EQ(u_binary_op::div, lhs_0.op);
        auto lhs_1 = std::get<simple_unit>(*lhs_0.lhs); // Ohm
        auto rhs_1 = std::get<simple_unit>(*lhs_0.rhs); // V

        EXPECT_EQ("Ohm", lhs_1.spelling);
        EXPECT_EQ(unit_pref::none, lhs_1.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_1.val.symbol);

        EXPECT_EQ("V", rhs_1.spelling);
        EXPECT_EQ(unit_pref::none, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::V, rhs_1.val.symbol);

        EXPECT_EQ("A", rhs_0.spelling);
        EXPECT_EQ(unit_pref::none, rhs_0.val.prefix);
        EXPECT_EQ(unit_sym::A, rhs_0.val.symbol);
    }
    {
        std::string unit = "Ohm/(V*A)";
        auto p = parser(unit);
        auto u = std::get<binary_unit>(*p.try_parse_unit().value());

        EXPECT_EQ(u_binary_op::div, u.op);
        auto lhs_0 = std::get<simple_unit>(*u.lhs); // Ohm
        auto rhs_0 = std::get<binary_unit>(*u.rhs); // V*A

        EXPECT_EQ(u_binary_op::mul, rhs_0.op);
        auto lhs_1 = std::get<simple_unit>(*rhs_0.lhs); // V
        auto rhs_1 = std::get<simple_unit>(*rhs_0.rhs); // A

        EXPECT_EQ("Ohm", lhs_0.spelling);
        EXPECT_EQ(unit_pref::none, lhs_0.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_0.val.symbol);

        EXPECT_EQ("V", lhs_1.spelling);
        EXPECT_EQ(unit_pref::none, lhs_1.val.prefix);
        EXPECT_EQ(unit_sym::V, lhs_1.val.symbol);

        EXPECT_EQ("A", rhs_1.spelling);
        EXPECT_EQ(unit_pref::none, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::A, rhs_1.val.symbol);
    }
    {
        std::vector<std::string> units = {
                "Ohm*identifier",
                "Ohm+identifier",
                "Ohm^identifier",
                "Ohm/identifier",
        };
        for (auto u: units) {
            auto p = parser(u);
            auto q = std::get<simple_unit>(*p.try_parse_unit().value());
            EXPECT_EQ("Ohm", q.spelling);
            EXPECT_EQ(unit_pref::none, q.val.prefix);
            EXPECT_EQ(unit_sym::Ohm, q.val.symbol);
        }
    }
    {
        std::vector<std::string> incorrect_units = {
            "Ohm^A",
            "V*2",
            "mA/-2",
            "2^uK",
            "pV^(2/mV)"
        };
        for (auto u: incorrect_units) {
            auto p = parser(u);
            EXPECT_THROW(p.try_parse_unit(), std::runtime_error);
        }
    }
    {
        std::vector<std::string> incorrect_units = {
            "-Ohm",
            "4.5*Ohm",
            "+V",
            "identifier",
            "7"
        };
        for (auto u: incorrect_units) {
            auto p = parser(u);
            EXPECT_FALSE(p.try_parse_unit());
        }
    }
}

TEST(parse, type) {
    {
        std::string type = "time";
        auto p = parser(type);
        auto q = std::get<quantity_type>(*p.parse_type());
        EXPECT_EQ(quantity::time, q.type);
    }
    {
        std::string type = "bar";
        auto p = parser(type);
        auto q = std::get<record_alias_type>(*p.parse_type());
        EXPECT_EQ("bar", q.name);
    }
    {
        std::string type = "voltage^2";
        auto p = parser(type);
        auto q = std::get<quantity_binary_type>(*p.parse_type());
        EXPECT_EQ(t_binary_op::pow, q.op);

        auto q_lhs = std::get<quantity_type>(*q.lhs);
        EXPECT_EQ(quantity::voltage, q_lhs.type);

        auto q_rhs = std::get<integer_type>(*q.rhs);
        EXPECT_EQ(2, q_rhs.val);
    }
    {
        std::string type = "voltage/conductance";
        auto p = parser(type);
        auto q = std::get<quantity_binary_type>(*p.parse_type());
        EXPECT_EQ(t_binary_op::div, q.op);

        auto q_lhs = std::get<quantity_type>(*q.lhs);
        EXPECT_EQ(quantity::voltage, q_lhs.type);

        auto q_rhs = std::get<quantity_type>(*q.rhs);
        EXPECT_EQ(quantity::conductance, q_rhs.type);
    }
    {
        std::string type = "current*time/voltage";
        auto p = parser(type);
        auto q = std::get<quantity_binary_type>(*p.parse_type());
        EXPECT_EQ(t_binary_op::div, q.op);

        auto q0 = std::get<quantity_binary_type>(*q.lhs);
        EXPECT_EQ(t_binary_op::mul, q0.op);

        auto q0_lhs = std::get<quantity_type>(*q0.lhs);
        EXPECT_EQ(quantity::current, q0_lhs.type);

        auto q0_rhs = std::get<quantity_type>(*q0.rhs);
        EXPECT_EQ(quantity::time, q0_rhs.type);

        auto q_rhs = std::get<quantity_type>(*q.rhs);
        EXPECT_EQ(quantity::voltage, q_rhs.type);
    }
    {
        std::string type = "current^-2/temperature*time^-1";
        auto p = parser(type);
        auto q = std::get<quantity_binary_type>(*p.parse_type());
        EXPECT_EQ(t_binary_op::mul, q.op);

        auto q0 = std::get<quantity_binary_type>(*q.lhs); // current^-2/temperature
        EXPECT_EQ(t_binary_op::div, q0.op);

        auto q1 = std::get<quantity_binary_type>(*q0.lhs); // current^-2
        EXPECT_EQ(t_binary_op::pow, q1.op);

        auto q1_lhs = std::get<quantity_type>(*q1.lhs); // current
        EXPECT_EQ(quantity::current, q1_lhs.type);

        auto q1_rhs = std::get<integer_type>(*q1.rhs); // -2
        EXPECT_EQ(-2, q1_rhs.val);

        auto q0_rhs = std::get<quantity_type>(*q0.rhs); // temperature
        EXPECT_EQ(quantity::temperature, q0_rhs.type);

        auto q2 = std::get<quantity_binary_type>(*q.rhs); // time^-1
        EXPECT_EQ(t_binary_op::pow, q2.op);

        auto q2_lhs = std::get<quantity_type>(*q2.lhs); // time
        EXPECT_EQ(quantity::time, q2_lhs.type);

        auto q2_rhs = std::get<integer_type>(*q2.rhs); // -1
        EXPECT_EQ(-1, q2_rhs.val);
    }
    {
        std::string type = "current^-2/(temperature*time^-1)";
        auto p = parser(type);
        auto q = std::get<quantity_binary_type>(*p.parse_type());
        EXPECT_EQ(t_binary_op::div, q.op);

        auto q0 = std::get<quantity_binary_type>(*q.lhs); // current^-2
        EXPECT_EQ(t_binary_op::pow, q0.op);

        auto q0_lhs = std::get<quantity_type>(*q0.lhs); // current
        EXPECT_EQ(quantity::current, q0_lhs.type);

        auto q0_rhs = std::get<integer_type>(*q0.rhs); // -2
        EXPECT_EQ(-2, q0_rhs.val);

        auto q1 = std::get<quantity_binary_type>(*q.rhs); // temperature*time^-1
        EXPECT_EQ(t_binary_op::mul, q1.op);

        auto q1_lhs = std::get<quantity_type>(*q1.lhs); // temperature
        EXPECT_EQ(quantity::temperature, q1_lhs.type);

        auto q2 = std::get<quantity_binary_type>(*q1.rhs); // time^-1
        EXPECT_EQ(t_binary_op::pow, q2.op);

        auto q2_lhs = std::get<quantity_type>(*q2.lhs); // time
        EXPECT_EQ(quantity::time, q2_lhs.type);

        auto q2_rhs = std::get<integer_type>(*q2.rhs); // -1
        EXPECT_EQ(-1, q2_rhs.val);
    }
    {
        std::string type = "{bar:voltage, baz:current^5}";
        auto p = parser(type);
        auto q = std::get<record_type>(*p.parse_type());
        EXPECT_EQ(2, q.fields.size());

        EXPECT_EQ("bar", q.fields[0].first);
        EXPECT_EQ("baz", q.fields[1].first);

        auto q0 = std::get<quantity_type>(*q.fields[0].second);
        EXPECT_EQ(quantity::voltage, q0.type);

        auto q1 = std::get<quantity_binary_type>(*q.fields[1].second);
        EXPECT_EQ(t_binary_op::pow, q1.op);

        auto q1_lhs = std::get<quantity_type>(*q1.lhs);
        EXPECT_EQ(quantity::current, q1_lhs.type);

        auto q1_rhs = std::get<integer_type>(*q1.rhs);
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
        auto e_iden = std::get<identifier_expr>(*p.parse_identifier());
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
        auto e_iden = std::get<identifier_expr>(*p.parse_typed_identifier());
        EXPECT_EQ("foo", e_iden.name);
        EXPECT_FALSE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);
    }
    {
        std::string identifier = "bar:time";
        auto p = parser(identifier);
        auto e_iden = std::get<identifier_expr>(*p.parse_typed_identifier());
        EXPECT_EQ("bar", e_iden.name);
        EXPECT_TRUE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);

        auto type = std::get<quantity_type>(*e_iden.type.value());
        EXPECT_EQ(quantity::time, type.type);
        EXPECT_EQ(src_location(1,5), type.loc);
    }
    {
        std::string identifier = "foo: bar";
        auto p = parser(identifier);
        auto e_iden = std::get<identifier_expr>(*p.parse_typed_identifier());
        EXPECT_EQ("foo", e_iden.name);
        EXPECT_TRUE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);

        auto type = std::get<record_alias_type>(*e_iden.type.value());
        EXPECT_EQ("bar", type.name);
        EXPECT_EQ(src_location(1,6), type.loc);
    }
    {
        std::string identifier = "foo: {bar:voltage, baz:current/time}";
        auto p = parser(identifier);
        auto e_iden = std::get<identifier_expr>(*p.parse_typed_identifier());
        EXPECT_EQ("foo", e_iden.name);
        EXPECT_TRUE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);

        auto type = std::get<record_type>(*e_iden.type.value());
        EXPECT_EQ(2, type.fields.size());
        EXPECT_EQ(src_location(1,6), type.loc);

        EXPECT_EQ("bar", type.fields[0].first);
        EXPECT_EQ("baz", type.fields[1].first);

        auto t_0 = std::get<quantity_type>(*type.fields[0].second);
        EXPECT_EQ(quantity::voltage, t_0.type);
        EXPECT_EQ(src_location(1, 11), t_0.loc);

        auto t_1 = std::get<quantity_binary_type>(*type.fields[1].second);
        EXPECT_EQ(t_binary_op::div, t_1.op);
        EXPECT_EQ(src_location(1, 31), t_1.loc);

        auto t_1_0 = std::get<quantity_type>(*t_1.lhs);
        EXPECT_EQ(quantity::current, t_1_0.type);

        auto t_1_1 = std::get<quantity_type>(*t_1.rhs);
        EXPECT_EQ(quantity::time, t_1_1.type);
    }
    {
        std::vector<std::string> invalid = {
                "a:1",
                "foo': /time",
                "bar: ",
                "bar: {a; b}",
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
        auto v = std::get<float_expr>(*p.parse_float());
        EXPECT_EQ(4.2, v.value);
        EXPECT_FALSE(v.unit);
    }
    {
        std::string fpt = "2.22 mV";
        auto p = parser(fpt);
        auto v = std::get<float_expr>(*p.parse_float());
        EXPECT_EQ(2.22, v.value);

        auto u = std::get<simple_unit>(*(v.unit.value()));
        EXPECT_EQ("mV", u.spelling);
        EXPECT_EQ(unit_pref::m, u.val.prefix);
        EXPECT_EQ(unit_sym::V, u.val.symbol);
    }
    {
        std::string fpt = "2e-4 A/s";
        auto p = parser(fpt);
        auto v = std::get<float_expr>(*p.parse_float());
        EXPECT_EQ(2e-4, v.value);

        auto u = std::get<binary_unit>(*(v.unit.value()));
        EXPECT_EQ(u_binary_op::div, u.op);

        auto u_lhs = std::get<simple_unit>(*u.lhs);
        EXPECT_EQ("A", u_lhs.spelling);
        EXPECT_EQ(unit_pref::none, u_lhs.val.prefix);
        EXPECT_EQ(unit_sym::A, u_lhs.val.symbol);

        auto u_rhs = std::get<simple_unit>(*u.rhs);
        EXPECT_EQ("s", u_rhs.spelling);
        EXPECT_EQ(unit_pref::none, u_rhs.val.prefix);
        EXPECT_EQ(unit_sym::s, u_rhs.val.symbol);
    }
    {
        std::string fpt = "2E2 Ohm^2";
        auto p = parser(fpt);
        auto v = std::get<float_expr>(*p.parse_float());
        EXPECT_EQ(2e2, v.value);

        auto u = std::get<binary_unit>(*(v.unit.value()));
        EXPECT_EQ(u_binary_op::pow, u.op);

        auto u_lhs = std::get<simple_unit>(*u.lhs);
        EXPECT_EQ("Ohm", u_lhs.spelling);
        EXPECT_EQ(unit_pref::none, u_lhs.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, u_lhs.val.symbol);

        auto u_rhs = std::get<integer_unit>(*u.rhs);
        EXPECT_EQ(2, u_rhs.val);
    }
}

TEST(parser, integer) {
    {
        std::string fpt = "4";
        auto p = parser(fpt);
        auto v = std::get<int_expr>(*p.parse_int());
        EXPECT_EQ(4, v.value);
        EXPECT_FALSE(v.unit);
    }
    {
        std::string fpt = "11 mV";
        auto p = parser(fpt);
        auto v = std::get<int_expr>(*p.parse_int());
        EXPECT_EQ(11, v.value);

        auto u = std::get<simple_unit>(*(v.unit.value()));
        EXPECT_EQ("mV", u.spelling);
        EXPECT_EQ(unit_pref::m, u.val.prefix);
        EXPECT_EQ(unit_sym::V, u.val.symbol);
    }
}

TEST(parser, call) {
    {
        std::string expr = "foo()";
        auto p = parser(expr);
        auto c = std::get<call_expr>(*p.parse_call());

        EXPECT_EQ("foo", c.function_name);
        EXPECT_EQ(0u, c.call_args.size());
        EXPECT_EQ(src_location(1, 1), c.loc);
    }
    {
        std::string expr = "foo(2, 1)";
        auto p = parser(expr);
        auto c = std::get<call_expr>(*p.parse_call());

        EXPECT_EQ("foo", c.function_name);
        EXPECT_EQ(2u, c.call_args.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg_0 = std::get<int_expr>(*c.call_args[0]);
        EXPECT_EQ(2, arg_0.value);
        EXPECT_FALSE(arg_0.unit);
        EXPECT_EQ(src_location(1, 5), arg_0.loc);

        auto arg_1 = std::get<int_expr>(*c.call_args[1]);
        EXPECT_EQ(1, arg_1.value);
        EXPECT_FALSE(arg_1.unit);
        EXPECT_EQ(src_location(1, 8), arg_1.loc);
    }
    {
        std::string expr = "foo_bar(2.5, a, -1 A)";
        auto p = parser(expr);
        auto c = std::get<call_expr>(*p.parse_call());

        EXPECT_EQ("foo_bar", c.function_name);
        EXPECT_EQ(3u, c.call_args.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg_0 = std::get<float_expr>(*c.call_args[0]);
        EXPECT_EQ(2.5, arg_0.value);
        EXPECT_FALSE(arg_0.unit);
        EXPECT_EQ(src_location(1, 9), arg_0.loc);

        auto arg_1 = std::get<identifier_expr>(*c.call_args[1]);
        EXPECT_EQ("a", arg_1.name);
        EXPECT_FALSE(arg_1.type);
        EXPECT_EQ(src_location(1, 14), arg_1.loc);

        auto arg_2 = std::get<unary_expr>(*c.call_args[2]);
        EXPECT_EQ(unary_op::neg, arg_2.op);
        EXPECT_EQ(src_location(1, 17), arg_2.loc);

        auto arg_2_v = std::get<int_expr>(*arg_2.value);
        EXPECT_EQ(1, arg_2_v.value);
        EXPECT_TRUE(arg_2_v.unit);
        auto unit = std::get<simple_unit>(*(arg_2_v.unit.value()));
        EXPECT_EQ("A", unit.spelling);
        EXPECT_EQ(unit_pref::none, unit.val.prefix);
        EXPECT_EQ(unit_sym::A, unit.val.symbol);
    }
    {
        std::string expr = "foo(1+4, bar())";
        auto p = parser(expr);
        auto c = std::get<call_expr>(*p.parse_call());

        EXPECT_EQ("foo", c.function_name);
        EXPECT_EQ(2u, c.call_args.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg0 = std::get<binary_expr>(*c.call_args[0]);
        EXPECT_EQ(binary_op::add, arg0.op);
        EXPECT_EQ(src_location(1, 6), arg0.loc);

        auto arg0_lhs = std::get<int_expr>(*arg0.lhs);
        EXPECT_EQ(1, arg0_lhs.value);
        EXPECT_FALSE(arg0_lhs.unit);

        auto arg0_rhs = std::get<int_expr>(*arg0.rhs);
        EXPECT_EQ(4, arg0_rhs.value);
        EXPECT_FALSE(arg0_rhs.unit);

        auto arg1 = std::get<call_expr>(*c.call_args[1]);
        EXPECT_EQ("bar", arg1.function_name);
        EXPECT_EQ(0u, arg1.call_args.size());
        EXPECT_EQ(src_location(1, 10), arg1.loc);
    }
    {
        std::string expr = "foo(let b: voltage = 6 mV; b, bar.X)";
        auto p = parser(expr);
        auto c = std::get<call_expr>(*p.parse_call());

        EXPECT_EQ("foo", c.function_name);
        EXPECT_EQ(2u, c.call_args.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg0 = std::get<let_expr>(*c.call_args[0]);
        EXPECT_EQ(src_location(1, 5), arg0.loc);

        auto arg0_id = std::get<identifier_expr>(*arg0.identifier);
        EXPECT_EQ("b", arg0_id.name);
        EXPECT_TRUE(arg0_id.type);
        auto arg0_type = std::get<quantity_type>(*(arg0_id.type.value()));
        EXPECT_EQ(quantity::voltage, arg0_type.type);

        auto arg0_v = std::get<int_expr>(*arg0.value);
        EXPECT_EQ(6, arg0_v.value);
        EXPECT_TRUE(arg0_v.unit);
        auto arg0_unit = std::get<simple_unit>(*(arg0_v.unit.value()));
        EXPECT_EQ(unit_pref::m, arg0_unit.val.prefix);
        EXPECT_EQ(unit_sym::V, arg0_unit.val.symbol);
        EXPECT_EQ("mV", arg0_unit.spelling);

        auto arg0_b = std::get<identifier_expr>(*arg0.body);
        EXPECT_EQ("b", arg0_b.name);
        EXPECT_FALSE(arg0_b.type);

        auto arg1 = std::get<binary_expr>(*c.call_args[1]);
        EXPECT_EQ(src_location(1, 34), arg1.loc);
        EXPECT_EQ(binary_op::dot, arg1.op);

        auto arg1_lhs = std::get<identifier_expr>(*arg1.lhs);
        EXPECT_EQ("bar", arg1_lhs.name);
        EXPECT_FALSE(arg1_lhs.type);

        auto arg1_rhs = std::get<identifier_expr>(*arg1.rhs);
        EXPECT_EQ("X", arg1_rhs.name);
        EXPECT_FALSE(arg1_rhs.type);
    }
}

TEST(parser, object) {
    {
        std::string obj = "bar{a = 0; b = 0;}";
        auto p = parser(obj);
        auto c = std::get<object_expr>(*p.parse_object());

        EXPECT_TRUE(c.record_name);
        EXPECT_EQ("bar", c.record_name.value());
        EXPECT_EQ(2u, c.record_fields.size());
        EXPECT_EQ(2u, c.record_values.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg_0 = std::get<identifier_expr>(*c.record_fields[0]);
        EXPECT_EQ("a", arg_0.name);
        EXPECT_FALSE(arg_0.type);
        auto val_0 = std::get<int_expr>(*c.record_values[0]);
        EXPECT_EQ(0, val_0.value);
        EXPECT_FALSE(val_0.unit);

        auto arg_1 = std::get<identifier_expr>(*c.record_fields[1]);
        EXPECT_EQ("b", arg_1.name);
        EXPECT_FALSE(arg_1.type);
        auto val_1 = std::get<int_expr>(*c.record_values[1]);
        EXPECT_EQ(0, val_1.value);
        EXPECT_FALSE(val_1.unit);
    }
    {
        std::string obj = "{a = 1; b = 2e-5;}";
        auto p = parser(obj);
        auto c = std::get<object_expr>(*p.parse_object());

        EXPECT_FALSE(c.record_name);
        EXPECT_EQ(2u, c.record_fields.size());
        EXPECT_EQ(2u, c.record_values.size());
        EXPECT_EQ(src_location(1, 1), c.loc);

        auto arg_0 = std::get<identifier_expr>(*c.record_fields[0]);
        EXPECT_EQ("a", arg_0.name);
        EXPECT_FALSE(arg_0.type);
        auto val_0 = std::get<int_expr>(*c.record_values[0]);
        EXPECT_EQ(1, val_0.value);
        EXPECT_FALSE(val_0.unit);

        auto arg_1 = std::get<identifier_expr>(*c.record_fields[1]);
        EXPECT_EQ("b", arg_1.name);
        EXPECT_FALSE(arg_1.type);
        auto val_1 = std::get<float_expr>(*c.record_values[1]);
        EXPECT_EQ(2e-5, val_1.value);
        EXPECT_FALSE(val_1.unit);
    }
}

TEST(parser, let) {
    {
        std::string let = "let foo = 9; 12.62";
        auto p = parser(let);
        auto e_let = std::get<let_expr>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e_let.loc);

        auto id = std::get<identifier_expr>(*e_let.identifier);
        EXPECT_EQ("foo", id.name);
        EXPECT_FALSE(id.type);
        EXPECT_EQ(src_location(1,5), id.loc);

        auto val = std::get<int_expr>(*e_let.value);
        EXPECT_EQ(9, val.value);
        EXPECT_FALSE(val.unit);
        EXPECT_EQ(src_location(1,11), val.loc);

        auto body = std::get<float_expr>(*e_let.body);
        EXPECT_EQ(12.62, body.value);
        EXPECT_FALSE(body.unit);
        EXPECT_EQ(src_location(1,14), body.loc);
    }
    {
        std::string let = "let g' = (exp(-g)); bar";
        auto p = parser(let);
        auto e_let = std::get<let_expr>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e_let.loc);

        auto id = std::get<identifier_expr>(*e_let.identifier);
        EXPECT_EQ("g'", id.name);
        EXPECT_FALSE(id.type);
        EXPECT_EQ(src_location(1,5), id.loc);

        auto val = std::get<unary_expr>(*e_let.value);
        EXPECT_EQ(unary_op::exp, val.op);
        EXPECT_FALSE(val.is_boolean());
        EXPECT_EQ(src_location(1,11), val.loc);

        auto val_0 = std::get<unary_expr>(*val.value);
        EXPECT_EQ(unary_op::neg, val_0.op);
        EXPECT_FALSE(val_0.is_boolean());
        EXPECT_EQ(src_location(1,15), val_0.loc);

        auto val_1 = std::get<identifier_expr>(*val_0.value);
        EXPECT_EQ("g", val_1.name);
        EXPECT_FALSE(val_1.type);
        EXPECT_EQ(src_location(1,16), val_1.loc);

        auto body = std::get<identifier_expr>(*e_let.body);
        EXPECT_EQ("bar", body.name);
        EXPECT_FALSE(body.type);
        EXPECT_EQ(src_location(1,21), body.loc);
    }
    {
        std::string let = "let a:voltage = -5; a + 3e5";
        auto p = parser(let);
        auto e_let = std::get<let_expr>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e_let.loc);

        auto id = std::get<identifier_expr>(*e_let.identifier);
        EXPECT_EQ("a", id.name);
        EXPECT_EQ(src_location(1,5), id.loc);

        auto type = std::get<quantity_type>(*id.type.value());
        EXPECT_EQ(quantity::voltage, type.type);
        EXPECT_EQ(src_location(1,7), type.loc);

        auto val = std::get<unary_expr>(*e_let.value);
        EXPECT_EQ(unary_op::neg, val.op);
        EXPECT_FALSE(val.is_boolean());
        EXPECT_EQ(src_location(1,17), val.loc);

        auto intval = std::get<int_expr>(*val.value);
        EXPECT_EQ(5, intval.value);
        EXPECT_FALSE(intval.unit);
        EXPECT_EQ(src_location(1,18), intval.loc);

        auto body = std::get<binary_expr>(*e_let.body);
        EXPECT_EQ(binary_op::add, body.op);
        EXPECT_FALSE(body.is_boolean());

        auto lhs = std::get<identifier_expr>(*body.lhs);
        EXPECT_EQ("a", lhs.name);
        EXPECT_FALSE(lhs.type);
        EXPECT_EQ(src_location(1,21), lhs.loc);

        auto rhs = std::get<float_expr>(*body.rhs);
        EXPECT_EQ(3e5, rhs.value);
        EXPECT_FALSE(rhs.unit);
        EXPECT_EQ(src_location(1,25), rhs.loc);
    }
    {
        std::string let = "let x = { a = 4; }.a; x";
        auto p = parser(let);
        auto e = std::get<let_expr>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_id = std::get<identifier_expr>(*e.identifier);
        EXPECT_EQ(src_location(1, 5), e_id.loc);
        EXPECT_EQ("x", e_id.name);
        EXPECT_FALSE(e_id.type);

        auto e_val = std::get<binary_expr>(*e.value);
        EXPECT_EQ(src_location(1, 19), e_val.loc);
        EXPECT_EQ(binary_op::dot, e_val.op);
        EXPECT_FALSE(e_val.is_boolean());

        auto e_val_lhs = std::get<object_expr>(*e_val.lhs);
        EXPECT_FALSE(e_val_lhs.record_name);
        EXPECT_EQ(1u, e_val_lhs.record_values.size());
        EXPECT_EQ(1u, e_val_lhs.record_fields.size());

        auto e_val_lhs_arg = std::get<identifier_expr>(*e_val_lhs.record_fields.front());
        EXPECT_EQ("a", e_val_lhs_arg.name);
        EXPECT_FALSE(e_val_lhs_arg.type);

        auto e_val_lhs_val = std::get<int_expr>(*e_val_lhs.record_values.front());
        EXPECT_EQ(4, e_val_lhs_val.value);
        EXPECT_FALSE(e_val_lhs_val.unit);

        auto e_val_rhs = std::get<identifier_expr>(*e_val.rhs);
        EXPECT_EQ("a", e_val_rhs.name);
        EXPECT_FALSE(e_val_rhs.type);

        auto e_body = std::get<identifier_expr>(*e.body);
        EXPECT_EQ(src_location(1, 23), e_body.loc);
        EXPECT_EQ("x", e_body.name);
        EXPECT_FALSE(e_body.type);

    }
    {
        std::string let = "let F = { u = G.t; }; F.u / 20 s";
        auto p = parser(let);
        auto e = std::get<let_expr>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_id = std::get<identifier_expr>(*e.identifier);
        EXPECT_EQ("F", e_id.name);
        EXPECT_FALSE(e_id.type);
        EXPECT_EQ(src_location(1,5), e_id.loc);

        auto e_val = std::get<object_expr>(*e.value);
        EXPECT_EQ(src_location(1,9), e_val.loc);
        EXPECT_FALSE(e_val.record_name);
        EXPECT_EQ(1u, e_val.record_values.size());
        EXPECT_EQ(1u, e_val.record_fields.size());

        auto e_val_arg = std::get<identifier_expr>(*e_val.record_fields.front());
        EXPECT_EQ("u", e_val_arg.name);
        EXPECT_FALSE(e_val_arg.type);

        auto e_val_val = std::get<binary_expr>(*e_val.record_values.front());
        EXPECT_EQ(binary_op::dot, e_val_val.op);

        auto e_val_lhs = std::get<identifier_expr>(*e_val_val.lhs);
        EXPECT_EQ("G", e_val_lhs.name);
        EXPECT_FALSE(e_val_lhs.type);
        auto e_val_rhs = std::get<identifier_expr>(*e_val_val.rhs);
        EXPECT_EQ("t", e_val_rhs.name);
        EXPECT_FALSE(e_val_rhs.type);

        auto e_body = std::get<binary_expr>(*e.body);
        EXPECT_EQ(src_location(1,27), e_body.loc);
        EXPECT_EQ(binary_op::div, e_body.op);

        auto e_body_lhs = std::get<binary_expr>(*e_body.lhs);
        EXPECT_EQ(binary_op::dot, e_body_lhs.op);

        auto e_body_lhs_lhs = std::get<identifier_expr>(*e_body_lhs.lhs);
        EXPECT_EQ("F", e_body_lhs_lhs.name);
        EXPECT_FALSE(e_body_lhs_lhs.type);

        auto e_body_lhs_rhs = std::get<identifier_expr>(*e_body_lhs.rhs);
        EXPECT_EQ("u", e_body_lhs_rhs.name);
        EXPECT_FALSE(e_body_lhs_rhs.type);

        auto e_body_rhs = std::get<int_expr>(*e_body.rhs);
        EXPECT_EQ(20, e_body_rhs.value);
        EXPECT_TRUE(e_body_rhs.unit);

        auto unit = std::get<simple_unit>(*(e_body_rhs.unit.value()));
        EXPECT_EQ(unit_pref::none, unit.val.prefix);
        EXPECT_EQ(unit_sym::s, unit.val.symbol);
    }
    {
        std::string let = "let a = 3 m;\n"
                          "let r = { a = 4; b = a; };\n"
                          "r.b";

        auto p = parser(let);
        auto e = std::get<let_expr>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_id = std::get<identifier_expr>(*e.identifier);
        EXPECT_EQ(src_location(1,5), e_id.loc);
        EXPECT_EQ("a", e_id.name);
        EXPECT_FALSE(e_id.type);

        auto e_val = std::get<int_expr>(*e.value);
        EXPECT_EQ(src_location(1,9), e_val.loc);
        EXPECT_EQ(3, e_val.value);
        EXPECT_TRUE(e_val.unit);
        auto unit = std::get<simple_unit>(*e_val.unit.value());
        EXPECT_EQ(unit_pref::none, unit.val.prefix);
        EXPECT_EQ(unit_sym::m, unit.val.symbol);

        auto e_body = std::get<let_expr>(*e.body);
        EXPECT_EQ(src_location(2,1), e_body.loc);

        auto e_body_id = std::get<identifier_expr>(*e_body.identifier);
        EXPECT_EQ(src_location(2,5), e_body_id.loc);
        EXPECT_EQ("r", e_body_id.name);
        EXPECT_FALSE(e_body_id.type);

        auto e_body_val = std::get<object_expr>(*e_body.value);
        EXPECT_EQ(src_location(2,9), e_body_val.loc);
        EXPECT_FALSE(e_body_val.record_name);

        auto e_body_val_arg0 = std::get<identifier_expr>(*e_body_val.record_fields[0]);
        EXPECT_EQ("a", e_body_val_arg0.name);
        EXPECT_FALSE(e_body_val_arg0.type);

        auto e_body_val_val0 = std::get<int_expr>(*e_body_val.record_values[0]);
        EXPECT_EQ(4, e_body_val_val0.value);
        EXPECT_FALSE(e_body_val_val0.unit);

        auto e_body_val_arg1 = std::get<identifier_expr>(*e_body_val.record_fields[1]);
        EXPECT_EQ("b", e_body_val_arg1.name);
        EXPECT_FALSE(e_body_val_arg1.type);

        auto e_body_val_val1 = std::get<identifier_expr>(*e_body_val.record_values[1]);
        EXPECT_EQ("a", e_body_val_val1.name);
        EXPECT_FALSE(e_body_val_val1.type);

        auto e_body_body = std::get<binary_expr>(*e_body.body);
        EXPECT_EQ(src_location(3,2), e_body_body.loc);
        EXPECT_EQ(binary_op::dot, e_body_body.op);

        auto e_body_body_lhs = std::get<identifier_expr>(*e_body_body.lhs);
        EXPECT_EQ("r", e_body_body_lhs.name);
        EXPECT_FALSE(e_body_body_lhs.type);

        auto e_body_body_rhs = std::get<identifier_expr>(*e_body_body.rhs);
        EXPECT_EQ("b", e_body_body_rhs.name);
        EXPECT_FALSE(e_body_body_rhs.type);
    }
/*    {
        std::string let = "let a = { scale = 3.2; pos = { x = 3 m ; y = 4 m; }; }; # binds `a` below.\n"
                          "with a.pos;   # binds `x` to 3 m and `y` to 4 m below.\n"
                          "a.scale*(x+y)";

        auto p = parser(let);
        auto e = std::get<let_expr>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e.loc);

        auto e_id = std::get<identifier_expr>(*e.identifier);
        EXPECT_EQ("a", e_id.name);
        EXPECT_FALSE(e_id.type);

        auto e_val = std::get<object_expr>(*e.value);
        EXPECT_FALSE(e_val.record_name);
        EXPECT_EQ(2u, e_val.record_fields.size());
        EXPECT_EQ(2u, e_val.record_values.size());

        auto e_val_arg0 = std::get<identifier_expr>(*e_val.record_fields[0]);
        EXPECT_EQ("scale", e_val_arg0.name);
        EXPECT_FALSE(e_val_arg0.type);

        auto e_val_val0 = std::get<float_expr>(*e_val.record_values[0]);
        EXPECT_EQ(3.2, e_val_val0.value);
        EXPECT_FALSE(e_val_val0.unit);

        auto e_val_arg1 = std::get<identifier_expr>(*e_val.record_fields[1]);
        EXPECT_EQ("pos", e_val_arg1.name);
        EXPECT_FALSE(e_val_arg1.type);

        auto e_val_val1 = std::get<object_expr>(*e_val.record_values[1]);
        EXPECT_FALSE(e_val_val1.record_name);
        EXPECT_EQ(2u, e_val_val1.record_fields.size());
        EXPECT_EQ(2u, e_val_val1.record_values.size());

        auto e_val_val1_arg0 = std::get<identifier_expr>(*e_val_val1.record_fields[0]);
        EXPECT_EQ("x", e_val_val1_arg0.name);
        EXPECT_FALSE(e_val_val1_arg0.type);

        auto e_val_val1_val0 = std::get<int_expr>(*e_val_val1.record_values[0]);
        EXPECT_EQ(3, e_val_val1_val0.value);
        EXPECT_TRUE(e_val_val1_val0.unit);
        auto unit0 = std::get<simple_unit>(*e_val_val1_val0.unit.value());
        EXPECT_EQ(unit_pref::none, unit0.val.prefix);
        EXPECT_EQ(unit_sym::m, unit0.val.symbol);

        auto e_val_val1_arg1 = std::get<identifier_expr>(*e_val_val1.record_fields[1]);
        EXPECT_EQ("y", e_val_val1_arg1.name);
        EXPECT_FALSE(e_val_val1_arg1.type);

        auto e_val_val1_val1 = std::get<int_expr>(*e_val_val1.record_values[1]);
        EXPECT_EQ(4, e_val_val1_val1.value);
        EXPECT_TRUE(e_val_val1_val1.unit);
        auto unit1 = std::get<simple_unit>(*e_val_val1_val1.unit.value());
        EXPECT_EQ(unit_pref::none, unit1.val.prefix);
        EXPECT_EQ(unit_sym::m, unit1.val.symbol);

        auto e_body = std::get<with_expr>(*e.body);
        auto e_body_id = std::get<binary_expr>(*e_body.identifier);

    }*/
    {
        std::string let = "let g = let a = v*v; a/p1; g*3";
    }
    {
        std::vector<std::string> invalid = {
            "let a:voltage = -5; a + ",
            "let a: = 3; 0",
            "let a = -1e5 0",
            "let _foo = 0; 0",
            "let foo = 0;",
        };
        for (const auto& s: invalid) {
            auto p = parser(s);
            EXPECT_THROW(p.parse_let(), std::runtime_error);
        }
    }
}
TEST(parser, with) {}
TEST(parser, conditional) {}
TEST(parser, unary_expr) {}
TEST(parser, binary_expr) {} // TODO 2.X? For `.` we can't have number expressions on the lhs, but maybe that's for type checking stage?
TEST(parser, parameter) {}
TEST(parser, constant) {}
TEST(parser, record) {}
TEST(parser, function) {}
TEST(parser, import) {}
TEST(parser, module) {}