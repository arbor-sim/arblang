#include <string>
#include <variant>

#include <arblang/token.hpp>
#include <arblang/parser.hpp>

#include "../gtest.h"

using namespace al;
using namespace raw_ir;
TEST(parser, value_expressions) {
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
        EXPECT_EQ("", val.unit);
        EXPECT_EQ(src_location(1,11), val.loc);

        auto body = std::get<float_expr>(*e_let.body);
        EXPECT_EQ(12.62, body.value);
        EXPECT_EQ("", body.unit);
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
        std::string let = "let a:voltage = -5; a + 3";
        auto p = parser(let);
        auto e_let = std::get<let_expr>(*p.parse_let());
        EXPECT_EQ(src_location(1,1), e_let.loc);

        auto id = std::get<identifier_expr>(*e_let.identifier);
        EXPECT_EQ("a", id.name);
        EXPECT_EQ(src_location(1,5), id.loc);

        auto type = std::get<t_raw_ir::quantity_type>(*id.type.value());
        EXPECT_EQ(t_raw_ir::quantity::voltage, type.type);
        EXPECT_EQ(src_location(1,7), type.loc);

        auto val = std::get<unary_expr>(*e_let.value);
        EXPECT_EQ(unary_op::neg, val.op);
        EXPECT_FALSE(val.is_boolean());
        EXPECT_EQ(src_location(1,17), val.loc);

        auto intval = std::get<int_expr>(*val.value);
        EXPECT_EQ(5, intval.value);
        EXPECT_EQ("", intval.unit);
        EXPECT_EQ(src_location(1,18), intval.loc);

        auto body = std::get<binary_expr>(*e_let.body);
        EXPECT_EQ(binary_op::add, body.op);
        EXPECT_FALSE(body.is_boolean());

        auto lhs = std::get<identifier_expr>(*body.lhs);
        EXPECT_EQ("a", lhs.name);
        EXPECT_FALSE(lhs.type);
        EXPECT_EQ(src_location(1,21), lhs.loc);

        auto rhs = std::get<int_expr>(*body.rhs);
        EXPECT_EQ(3, rhs.value);
        EXPECT_EQ("", rhs.unit);
        EXPECT_EQ(src_location(1,25), rhs.loc);
    }
}