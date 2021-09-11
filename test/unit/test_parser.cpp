#include <string>
#include <variant>
#include <vector>

#include <arblang/token.hpp>
#include <arblang/parser.hpp>

#include "../gtest.h"

using namespace al;
using namespace raw_ir;

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

        auto type = std::get<t_raw_ir::quantity_type>(*e_iden.type.value());
        EXPECT_EQ(t_raw_ir::quantity::time, type.type);
        EXPECT_EQ(src_location(1,5), type.loc);
    }
    {
        std::string identifier = "foo: bar";
        auto p = parser(identifier);
        auto e_iden = std::get<identifier_expr>(*p.parse_typed_identifier());
        EXPECT_EQ("foo", e_iden.name);
        EXPECT_TRUE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);

        auto type = std::get<t_raw_ir::record_alias_type>(*e_iden.type.value());
        EXPECT_EQ("bar", type.name);
        EXPECT_EQ(src_location(1,6), type.loc);
    }
    {
        std::string identifier = "foo: {bar:voltage; baz:current/time}";
        auto p = parser(identifier);
        auto e_iden = std::get<identifier_expr>(*p.parse_typed_identifier());
        EXPECT_EQ("bar", e_iden.name);
        EXPECT_TRUE(e_iden.type);
        EXPECT_EQ(src_location(1,1), e_iden.loc);

        auto type = std::get<t_raw_ir::record_type>(*e_iden.type.value());
        EXPECT_EQ(2, type.fields.size());
        EXPECT_EQ(src_location(1,6), type.loc);

        EXPECT_EQ("bar", type.fields[0].first);
        EXPECT_EQ("baz", type.fields[1].first);

        auto t_0 = std::get<t_raw_ir::quantity_type>(*type.fields[0].second);
        EXPECT_EQ(t_raw_ir::quantity::voltage, t_0.type);
        EXPECT_EQ(src_location(1, 11), t_0.loc);

        auto t_1 = std::get<t_raw_ir::quantity_binary_type>(*type.fields[1].second);
        EXPECT_EQ(t_raw_ir::t_binary_op::div, t_1.op);
        EXPECT_EQ(src_location(1, 24), t_1.loc);

        auto t_1_0 = std::get<t_raw_ir::quantity_type>(*t_1.lhs);
        EXPECT_EQ(t_raw_ir::quantity::current, t_1_0.type);

        auto t_1_1 = std::get<t_raw_ir::quantity_type>(*t_1.lhs);
        EXPECT_EQ(t_raw_ir::quantity::time, t_1_1.type);
    }
    {
        std::vector<std::string> invalid = {
                "a:1",
                "foo': /time",
                "bar: ",
                "bar: {a; b}",
                "baz_ voltage",
        };
        for (const auto& s: invalid) {
            auto p = parser(s);
            EXPECT_THROW(p.parse_typed_identifier(), std::runtime_error);
        }
    }
}
TEST(parser, float_pt) {}
TEST(parser, integer) {}
TEST(parser, call) {}
TEST(parser, field) {}
TEST(parser, object) {}
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
        std::string let = "let a:voltage = -5; a + 3e5";
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

        auto rhs = std::get<float_expr>(*body.rhs);
        EXPECT_EQ(3e5, rhs.value);
        EXPECT_EQ("", rhs.unit);
        EXPECT_EQ(src_location(1,25), rhs.loc);
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
TEST(parser, parameter) {}
TEST(parser, constant) {}
TEST(parser, record) {}
TEST(parser, function) {}
TEST(parser, import) {}
TEST(parser, module) {}
TEST(parser, type) {}