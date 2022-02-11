#include <cmath>
#include <string>
#include <variant>

#include <arblang/parser/parser.hpp>
#include <arblang/parser/normalizer.hpp>

#include "../gtest.h"

using namespace al;
using namespace parsed_ir;

// TODO test exceptions properly

TEST(normalizer, unit) {
    using namespace parsed_unit_ir;
    {
        std::string unit = "[mV]";
        auto p = parser(unit);
        auto normalized = normalize_unit(p.try_parse_unit());
        auto u = std::get<parsed_simple_unit>(*normalized.first);
        auto factor = normalized.second;

        EXPECT_EQ(-3, factor);
        EXPECT_EQ(unit_pref::none, u.val.prefix);
        EXPECT_EQ(unit_sym::V, u.val.symbol);
    }
    {
        std::string unit = "[mmol/kA]";
        auto p = parser(unit);
        auto normalized = normalize_unit(p.try_parse_unit());
        auto u = std::get<parsed_binary_unit>(*normalized.first);
        auto factor = normalized.second;
        EXPECT_EQ(factor, -6);

        auto lhs = std::get<parsed_simple_unit>(*u.lhs);
        auto rhs = std::get<parsed_simple_unit>(*u.rhs);

        EXPECT_EQ(unit_pref::none, lhs.val.prefix);
        EXPECT_EQ(unit_sym::mol, lhs.val.symbol);

        EXPECT_EQ(unit_pref::none, rhs.val.prefix);
        EXPECT_EQ(unit_sym::A, rhs.val.symbol);

        EXPECT_EQ(u_binary_op::div, u.op);
    }
    {
        std::string unit = "[K^-2]";
        auto p = parser(unit);
        auto normalized = normalize_unit(p.try_parse_unit());
        auto u = std::get<parsed_binary_unit>(*normalized.first);
        auto factor = normalized.second;
        EXPECT_EQ(factor, 0);

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
        auto normalized = normalize_unit(p.try_parse_unit());
        auto u = std::get<parsed_binary_unit>(*normalized.first);
        auto factor = normalized.second;
        EXPECT_EQ(factor, -30);

        EXPECT_EQ(u_binary_op::div, u.op);
        auto lhs_0 = std::get<parsed_binary_unit>(*u.lhs); // Ohm*uV
        auto rhs_0 = std::get<parsed_simple_unit>(*u.rhs); // YS

        EXPECT_EQ(u_binary_op::mul, lhs_0.op);
        auto lhs_1 = std::get<parsed_simple_unit>(*lhs_0.lhs); // Ohm
        auto rhs_1 = std::get<parsed_simple_unit>(*lhs_0.rhs); // uV

        EXPECT_EQ(unit_pref::none, lhs_1.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_1.val.symbol);

        EXPECT_EQ(unit_pref::none, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::V, rhs_1.val.symbol);

        EXPECT_EQ(unit_pref::none, rhs_0.val.prefix);
        EXPECT_EQ(unit_sym::S, rhs_0.val.symbol);
    }
    {
        std::string unit = "[kOhm^2/daC/mK^-3]";
        auto p = parser(unit);
        auto normalized = normalize_unit(p.try_parse_unit());
        auto u = std::get<parsed_binary_unit>(*normalized.first);
        auto factor = normalized.second;
        EXPECT_EQ(factor, -4);

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

        EXPECT_EQ(unit_pref::none, rhs_1.val.prefix);
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
        auto normalized = normalize_unit(p.try_parse_unit());
        auto u = std::get<parsed_binary_unit>(*normalized.first);
        auto factor = normalized.second;
        EXPECT_EQ(factor, 2);

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

        EXPECT_EQ(unit_pref::none, rhs_1.val.prefix);
        EXPECT_EQ(unit_sym::C, rhs_1.val.symbol);

        EXPECT_EQ(unit_pref::none, lhs_2.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, lhs_2.val.symbol);

        EXPECT_EQ(2, rhs_2.val);

        EXPECT_EQ(unit_pref::none, lhs_3.val.prefix);
        EXPECT_EQ(unit_sym::K, lhs_3.val.symbol);

        EXPECT_EQ(-1, rhs_3.val);
    }
}

TEST(normalizer, number_expr) {
    {
        std::string fpt = "2.22 [mV]";
        auto p = parser(fpt);
        auto normalized = normalize(p.parse_float());
        auto v = std::get<parsed_float>(*normalized);
        EXPECT_EQ(2.22e-3, v.value);

        auto u = std::get<parsed_simple_unit>(*v.unit);
        EXPECT_EQ(unit_pref::none, u.val.prefix);
        EXPECT_EQ(unit_sym::V, u.val.symbol);
    }
    {
        std::string fpt = "2e-4 [kA/s]";
        auto p = parser(fpt);
        auto normalized = normalize(p.parse_float());
        auto v = std::get<parsed_float>(*normalized);
        EXPECT_EQ(2e-1, v.value);

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
        std::string fpt = "2000 [dOhm^3]";
        auto p = parser(fpt);
        auto normalized = normalize(p.parse_int());
        auto v = std::get<parsed_int>(*normalized);
        EXPECT_EQ(2, v.value);

        auto u = std::get<parsed_binary_unit>(*v.unit);
        EXPECT_EQ(u_binary_op::pow, u.op);

        auto u_lhs = std::get<parsed_simple_unit>(*u.lhs);
        EXPECT_EQ(unit_pref::none, u_lhs.val.prefix);
        EXPECT_EQ(unit_sym::Ohm, u_lhs.val.symbol);

        auto u_rhs = std::get<parsed_integer_unit>(*u.rhs);
        EXPECT_EQ(3, u_rhs.val);
    }
    {
        std::string fpt = "1.09 [nA/um^2]";
        auto p = parser(fpt);
        auto normalized = normalize(p.parse_float());
        auto v = std::get<parsed_int>(*normalized);
        EXPECT_EQ(1090, v.value);

        auto u = std::get<parsed_binary_unit>(*v.unit);
        EXPECT_EQ(u_binary_op::div, u.op);

        auto u_lhs = std::get<parsed_simple_unit>(*u.lhs);
        EXPECT_EQ(unit_pref::none, u_lhs.val.prefix);
        EXPECT_EQ(unit_sym::A, u_lhs.val.symbol);

        auto u_rhs = std::get<parsed_binary_unit>(*u.rhs);
        EXPECT_EQ(u_binary_op::pow, u_rhs.op);

        auto u_rhs_lhs = std::get<parsed_simple_unit>(*u_rhs.lhs);
        EXPECT_EQ(unit_pref::none, u_rhs_lhs.val.prefix);
        EXPECT_EQ(unit_sym::m, u_rhs_lhs.val.symbol);

        auto u_rhs_rhs = std::get<parsed_integer_unit>(*u_rhs.rhs);
        EXPECT_EQ(2, u_rhs_rhs.val);
    }
    {
        std::string fpt = "10 [nA/um]";
        auto p = parser(fpt);
        auto normalized = normalize(p.parse_int());
        auto v = std::get<parsed_float>(*normalized);
        EXPECT_EQ(0.01, v.value);

        auto u = std::get<parsed_binary_unit>(*v.unit);
        EXPECT_EQ(u_binary_op::div, u.op);

        auto u_lhs = std::get<parsed_simple_unit>(*u.lhs);
        EXPECT_EQ(unit_pref::none, u_lhs.val.prefix);
        EXPECT_EQ(unit_sym::A, u_lhs.val.symbol);

        auto u_rhs = std::get<parsed_simple_unit>(*u.rhs);
        EXPECT_EQ(unit_pref::none, u_rhs.val.prefix);
        EXPECT_EQ(unit_sym::m, u_rhs.val.symbol);
    }
    {
        std::string fpt = "1";
        auto p = parser(fpt);
        auto normalized = normalize(p.parse_int());
        auto v = std::get<parsed_int>(*normalized);
        EXPECT_EQ(1, v.value);
    }
}
