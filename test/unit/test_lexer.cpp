#include <string>

#include <arblang/token.hpp>
#include <arblang/lexer.hpp>

#include "../gtest.h"

using namespace al;

TEST(lexer, symbols) {
    {
        std::string symbols = "foo\nbuzz, f_zz; foo' Foo'_ Foo'bar foo_Bar'_' ";
        auto lex = lexer(symbols.c_str());

        auto t1 = lex.current();
        EXPECT_EQ(tok::identifier, t1.type);
        EXPECT_EQ("foo", t1.spelling);

        auto t2 = lex.next();
        EXPECT_EQ(tok::identifier, t2.type);
        EXPECT_EQ("buzz", t2.spelling);

        auto t3 = lex.next();
        EXPECT_EQ(tok::comma, t3.type);

        auto t4 = lex.next();
        EXPECT_EQ(tok::identifier, t4.type);
        EXPECT_EQ("f_zz", t4.spelling);

        auto t5 = lex.next();
        EXPECT_EQ(tok::semicolon, t5.type);

        auto t6 = lex.next();
        EXPECT_EQ(tok::identifier, t6.type);
        EXPECT_EQ("foo'", t6.spelling);

        auto t7 = lex.next();
        EXPECT_EQ(tok::identifier, t7.type);
        EXPECT_EQ("Foo'_", t7.spelling);

        auto t8 = lex.next();
        EXPECT_EQ(tok::identifier, t8.type);
        EXPECT_EQ("Foo'bar", t8.spelling);

        auto t9 = lex.next();
        EXPECT_EQ(tok::identifier, t9.type);
        EXPECT_EQ("foo_Bar'_'", t9.spelling);

        auto tlast = lex.next();
        EXPECT_EQ(tok::eof, tlast.type);
    }
    {
        std::string error = "_foo ";
        auto t1 = lexer(error.c_str()).current();
        EXPECT_EQ(tok::error, t1.type);
    }
}

TEST(lexer, keywords) {
    std::string symbols = "if else parameter import let with length time conductance";
    auto lex = lexer(symbols.c_str());

    // should skip all white space and go straight to eof
    auto t1 = lex.current();
    EXPECT_EQ(tok::if_stmt, t1.type);
    EXPECT_EQ("if", t1.spelling);

    auto t2 = lex.next();
    EXPECT_EQ(tok::else_stmt, t2.type);
    EXPECT_EQ("else", t2.spelling);

    auto t3 = lex.next();
    EXPECT_EQ(tok::parameter, t3.type);
    EXPECT_EQ("parameter", t3.spelling);

    auto t4 = lex.next();
    EXPECT_EQ(tok::import, t4.type);
    EXPECT_EQ("import", t4.spelling);

    auto t5 = lex.next();
    EXPECT_EQ(tok::let, t5.type);
    EXPECT_EQ("let", t5.spelling);

    auto t6 = lex.next();
    EXPECT_EQ(tok::with, t6.type);
    EXPECT_EQ("with", t6.spelling);

    auto t7 = lex.next();
    EXPECT_EQ(tok::length, t7.type);
    EXPECT_EQ("length", t7.spelling);

    auto t8 = lex.next();
    EXPECT_EQ(tok::time, t8.type);
    EXPECT_EQ("time", t8.spelling);

    auto t9 = lex.next();
    EXPECT_EQ(tok::conductance, t9.type);
    EXPECT_EQ("conductance", t9.spelling);

    auto tlast = lex.next();
    EXPECT_EQ(tok::eof, tlast.type);
}

TEST(lexer, whitespace) {
    std::string whitespace = " \t\v\f";
    auto lex = lexer(whitespace.c_str());

    auto t1 = lex.current();
    EXPECT_EQ(tok::eof, t1.type);
}

TEST(lexer, newline) {
    {
        std::string str = "foo \n    bar \n +\r\n-";
        auto lex = lexer(str.c_str());

        // get foo
        auto t1 = lex.current();
        EXPECT_EQ(tok::identifier, t1.type);
        EXPECT_EQ("foo", t1.spelling);
        EXPECT_EQ(1, t1.loc.line);
        EXPECT_EQ(1, t1.loc.column);

        auto t2 = lex.next();
        EXPECT_EQ(tok::identifier, t2.type);
        EXPECT_EQ("bar", t2.spelling);
        EXPECT_EQ(2, t2.loc.line);
        EXPECT_EQ(5, t2.loc.column);

        auto t3 = lex.next();
        EXPECT_EQ(tok::plus, t3.type);
        EXPECT_EQ("+", t3.spelling);
        EXPECT_EQ(3, t3.loc.line);
        EXPECT_EQ(2, t3.loc.column);

        // test for carriage return + newline, i.e. \r\n
        auto t4 = lex.next();
        EXPECT_EQ(tok::minus, t4.type);
        EXPECT_EQ("-", t4.spelling);
        EXPECT_EQ(4, t4.loc.line);
        EXPECT_EQ(1, t4.loc.column);
    }
    {
        std::string error = " \r";
        auto lex = lexer(error.c_str());

        auto t1 = lex.current();
        EXPECT_EQ(tok::error, t1.type);
    }
}

TEST(lexer, operators) {
    std::string ops = "+-/*, t= ^ h'<->->";
    auto lex = lexer(ops.c_str());

    auto t1 = lex.current();
    EXPECT_EQ(tok::plus, t1.type);

    auto t2 = lex.next();
    EXPECT_EQ(tok::minus, t2.type);

    auto t3 = lex.next();
    EXPECT_EQ(tok::divide, t3.type);

    auto t4 = lex.next();
    EXPECT_EQ(tok::times, t4.type);

    auto t5 = lex.next();
    EXPECT_EQ(tok::comma, t5.type);

    // test that identifier followed by = is parsed correctly
    auto t6 = lex.next();
    EXPECT_EQ(tok::identifier, t6.type);

    auto t7 = lex.next();
    EXPECT_EQ(tok::eq, t7.type);

    auto t8 = lex.next();
    EXPECT_EQ(tok::pow, t8.type);

    auto t9 = lex.next();
    EXPECT_EQ(tok::identifier, t9.type);
    EXPECT_EQ("h'", t9.spelling);

    auto t11 = lex.next();
    EXPECT_EQ(tok::arrow, t11.type);

    auto t12 = lex.next();
    EXPECT_EQ(tok::ret, t12.type);

    auto tlast = lex.next();
    EXPECT_EQ(tok::eof, tlast.type);
}

TEST(lexer, comparison_operators) {
    {
        std::string ops = "< <= > >= == != && || !";
        auto lex = lexer(ops.c_str());

        auto t1 = lex.current();
        EXPECT_EQ(tok::lt, t1.type);

        auto t2 = lex.next();
        EXPECT_EQ(tok::le, t2.type);

        auto t3 = lex.next();
        EXPECT_EQ(tok::gt, t3.type);

        auto t4 = lex.next();
        EXPECT_EQ(tok::ge, t4.type);

        auto t5 = lex.next();
        EXPECT_EQ(tok::equality, t5.type);

        auto t6 = lex.next();
        EXPECT_EQ(tok::ne, t6.type);

        auto t7 = lex.next();
        EXPECT_EQ(tok::land, t7.type);

        auto t8 = lex.next();
        EXPECT_EQ(tok::lor, t8.type);

        auto t9 = lex.next();
        EXPECT_EQ(tok::lnot, t9.type);

        auto tlast = lex.next();
        EXPECT_EQ(tok::eof, tlast.type);
    }
    {
        auto lex = lexer("&");
        EXPECT_EQ(tok::error, lex.current().type);
    }
    {
        auto lex = lexer("&&&");
        EXPECT_EQ(tok::error, lex.next().type);
    }
    {
        auto lex = lexer("|");
        EXPECT_EQ(tok::error, lex.current().type);
    }
    {
        auto lex = lexer("|||");
        EXPECT_EQ(tok::error, lex.next().type);
    }
}

TEST(lexer, braces) {
    std::string str = "foo}";
    auto lex = lexer(str.c_str());

    auto t1 = lex.current();
    EXPECT_EQ(tok::identifier, t1.type);

    auto t2 = lex.next();
    EXPECT_EQ(tok::rbrace, t2.type);

    auto t3 = lex.next();
    EXPECT_EQ(tok::eof, t3.type);
}

TEST(lexer, comments) {
    std::string comments = "foo#this is one line\n"
                           "bar # another comment\n"
                           "#comments\n";
    auto lex = lexer(comments.c_str());

    auto t1 = lex.current();
    EXPECT_EQ(tok::identifier, t1.type);

    auto t2 = lex.next();
    EXPECT_EQ(tok::identifier, t2.type);
    EXPECT_EQ(2, t2.loc.line);

    auto t3 = lex.next();
    EXPECT_EQ(tok::eof, t3.type);
}

TEST(lexer, numbers) {
    {
        auto numeric = [](tok t) { return t == tok::floatpt || t == tok::integer; };
        std::string floats_stream = "1 23 .3 87.99 12. 1.e3 1.2e+2 23e-3 -3";
        auto lex = lexer(floats_stream.c_str());

        std::vector<double> floats = {1, 23, .3, 87.99, 12., 1.e3, 1.2E+2, 23e-3, -3};
        std::vector<long long> ints = {1, 23, 3};
        std::vector<long long> lexer_ints;

        auto t = lex.current();
        auto iter = floats.cbegin();
        while (t.type != tok::eof && iter != floats.cend()) {
            if (*iter < 0) {
                // the lexer does not decide where the - sign goes
                // the parser uses additional contextual information to
                // decide if the minus is a binary or unary expression
                EXPECT_EQ(tok::minus, t.type);
                t = lex.next();
            }
            EXPECT_TRUE(numeric(t.type));
            if (t.type == tok::integer) lexer_ints.push_back(std::stoll(t.spelling));
            EXPECT_EQ(std::abs(*iter), std::stod(t.spelling));

            ++iter;
            t = lex.next();
        }

        EXPECT_EQ(floats.cend(), iter);
        EXPECT_EQ(tok::eof, t.type);
        EXPECT_EQ(lexer_ints, ints);
    }

    {
        // check case where 'E' is not followed by +, -, or a digit explicitly
        auto lex = lexer("7.2E");
        auto t0 = lex.current();
        EXPECT_EQ(tok::floatpt, t0.type);
        EXPECT_EQ("7.2", t0.spelling);

        auto t1 = lex.next();
        EXPECT_EQ(tok::identifier, t1.type);
        EXPECT_EQ("E", t1.spelling);
    }
    {
        auto lex = lexer("3E+E2");
        auto t0 = lex.current();
        EXPECT_EQ(tok::integer, t0.type);
        EXPECT_EQ("3", t0.spelling);

        auto t1 = lex.next();
        EXPECT_EQ(tok::identifier, t1.type);
        EXPECT_EQ("E", t1.spelling);

        auto t2 = lex.next();
        EXPECT_EQ(tok::plus, t2.type);
        EXPECT_EQ("+", t2.spelling);

        auto t3 = lex.next();
        EXPECT_EQ(tok::identifier, t3.type);
        EXPECT_EQ("E2", t3.spelling);
    }
    {
        auto lex = lexer("1.2.3");
        EXPECT_EQ(tok::error, lex.current().type);
    }
    {
        auto lex = lexer("1.2E4.3");
        EXPECT_EQ(tok::error, lex.current().type);
    }
}