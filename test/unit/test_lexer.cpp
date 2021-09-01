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
        EXPECT_EQ(t1.type, tok::identifier);
        EXPECT_EQ(t1.spelling, "foo");

        auto t2 = lex.next();
        EXPECT_EQ(t2.type, tok::identifier);
        EXPECT_EQ(t2.spelling, "buzz");

        auto t3 = lex.next();
        EXPECT_EQ(t3.type, tok::comma);

        auto t4 = lex.next();
        EXPECT_EQ(t4.type, tok::identifier);
        EXPECT_EQ(t4.spelling, "f_zz");

        auto t5 = lex.next();
        EXPECT_EQ(t5.type, tok::semicolon);

        auto t6 = lex.next();
        EXPECT_EQ(t6.type, tok::identifier);
        EXPECT_EQ(t6.spelling, "foo'");

        auto t7 = lex.next();
        EXPECT_EQ(t7.type, tok::identifier);
        EXPECT_EQ(t7.spelling, "Foo'_");

        auto t8 = lex.next();
        EXPECT_EQ(t8.type, tok::identifier);
        EXPECT_EQ(t8.spelling, "Foo'bar");

        auto t9 = lex.next();
        EXPECT_EQ(t9.type, tok::identifier);
        EXPECT_EQ(t9.spelling, "foo_Bar'_'");

        auto tlast = lex.next();
        EXPECT_EQ(tlast.type, tok::eof);
    }
    {
        std::string error = "_foo ";
        auto lex = lexer(error.c_str());

        auto t1 = lex.current();
        EXPECT_EQ(t1.type, tok::error);
    }
}

TEST(lexer, keywords) {
    std::string symbols = "if else parameter import let with length time conductance";
    auto lex = lexer(symbols.c_str());

    // should skip all white space and go straight to eof
    auto t1 = lex.current();
    EXPECT_EQ(t1.type, tok::if_stmt);
    EXPECT_EQ(t1.spelling, "if");

    auto t2 = lex.next();
    EXPECT_EQ(t2.type, tok::else_stmt);
    EXPECT_EQ(t2.spelling, "else");

    auto t3 = lex.next();
    EXPECT_EQ(t3.type, tok::parameter);
    EXPECT_EQ(t3.spelling, "parameter");

    auto t4 = lex.next();
    EXPECT_EQ(t4.type, tok::import);
    EXPECT_EQ(t4.spelling, "import");

    auto t5 = lex.next();
    EXPECT_EQ(t5.type, tok::let);
    EXPECT_EQ(t5.spelling, "let");

    auto t6 = lex.next();
    EXPECT_EQ(t6.type, tok::with);
    EXPECT_EQ(t6.spelling, "with");

    auto t7 = lex.next();
    EXPECT_EQ(t7.type, tok::length);
    EXPECT_EQ(t7.spelling, "length");

    auto t8 = lex.next();
    EXPECT_EQ(t8.type, tok::time);
    EXPECT_EQ(t8.spelling, "time");

    auto t9 = lex.next();
    EXPECT_EQ(t9.type, tok::conductance);
    EXPECT_EQ(t9.spelling, "conductance");

    auto tlast = lex.next();
    EXPECT_EQ(tlast.type, tok::eof);
}

TEST(lexer, whitespace) {
    std::string whitespace = " \t\v\f";
    auto lex = lexer(whitespace.c_str());

    auto t1 = lex.current();
    EXPECT_EQ(t1.type, tok::eof);
}

TEST(lexer, newline) {
    {
        std::string str = "foo \n    bar \n +\r\n-";
        auto lex = lexer(str.c_str());

        // get foo
        auto t1 = lex.current();
        EXPECT_EQ(t1.type, tok::identifier);
        EXPECT_EQ(t1.spelling, "foo");
        EXPECT_EQ(t1.loc.line, 1);
        EXPECT_EQ(t1.loc.column, 1);

        auto t2 = lex.next();
        EXPECT_EQ(t2.type, tok::identifier);
        EXPECT_EQ(t2.spelling, "bar");
        EXPECT_EQ(t2.loc.line, 2);
        EXPECT_EQ(t2.loc.column, 5);

        auto t3 = lex.next();
        EXPECT_EQ(t3.type, tok::plus);
        EXPECT_EQ(t3.spelling, "+");
        EXPECT_EQ(t3.loc.line, 3);
        EXPECT_EQ(t3.loc.column, 2);

        // test for carriage return + newline, i.e. \r\n
        auto t4 = lex.next();
        EXPECT_EQ(t4.type, tok::minus);
        EXPECT_EQ(t4.spelling, "-");
        EXPECT_EQ(t4.loc.line, 4);
        EXPECT_EQ(t4.loc.column, 1);
    }
    {
        std::string error = " \r";
        auto lex = lexer(error.c_str());

        auto t1 = lex.current();
        EXPECT_EQ(t1.type, tok::error);
    }
}

TEST(lexer, operators) {
    std::string ops = "+-/*, t= ^ h'<->->";
    auto lex = lexer(ops.c_str());

    auto t1 = lex.current();
    EXPECT_EQ(t1.type, tok::plus);

    auto t2 = lex.next();
    EXPECT_EQ(t2.type, tok::minus);

    auto t3 = lex.next();
    EXPECT_EQ(t3.type, tok::divide);

    auto t4 = lex.next();
    EXPECT_EQ(t4.type, tok::times);

    auto t5 = lex.next();
    EXPECT_EQ(t5.type, tok::comma);

    // test that identifier followed by = is parsed correctly
    auto t6 = lex.next();
    EXPECT_EQ(t6.type, tok::identifier);

    auto t7 = lex.next();
    EXPECT_EQ(t7.type, tok::eq);

    auto t8 = lex.next();
    EXPECT_EQ(t8.type, tok::pow);

    auto t9 = lex.next();
    EXPECT_EQ(t9.type, tok::identifier);
    EXPECT_EQ(t9.spelling, "h'");

    auto t11 = lex.next();
    EXPECT_EQ(t11.type, tok::arrow);

    auto t12 = lex.next();
    EXPECT_EQ(t12.type, tok::ret);

    auto tlast = lex.next();
    EXPECT_EQ(tlast.type, tok::eof);
}

TEST(lexer, comparison_operators) {
    {
        std::string ops = "< <= > >= == != && || !";
        auto lex = lexer(ops.c_str());

        auto t1 = lex.current();
        EXPECT_EQ(t1.type, tok::lt);

        auto t2 = lex.next();
        EXPECT_EQ(t2.type, tok::le);

        auto t3 = lex.next();
        EXPECT_EQ(t3.type, tok::gt);

        auto t4 = lex.next();
        EXPECT_EQ(t4.type, tok::ge);

        auto t5 = lex.next();
        EXPECT_EQ(t5.type, tok::equality);

        auto t6 = lex.next();
        EXPECT_EQ(t6.type, tok::ne);

        auto t7 = lex.next();
        EXPECT_EQ(t7.type, tok::land);

        auto t8 = lex.next();
        EXPECT_EQ(t8.type, tok::lor);

        auto t9 = lex.next();
        EXPECT_EQ(t9.type, tok::lnot);

        auto tlast = lex.next();
        EXPECT_EQ(tlast.type, tok::eof);
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
    EXPECT_EQ(t1.type, tok::identifier);

    auto t2 = lex.next();
    EXPECT_EQ(t2.type, tok::rbrace);

    auto t3 = lex.next();
    EXPECT_EQ(t3.type, tok::eof);
}

TEST(lexer, comments) {
    std::string comments = "foo#this is one line\n"
                           "bar # another comment\n"
                           "#comments\n";
    auto lex = lexer(comments.c_str());

    auto t1 = lex.current();
    EXPECT_EQ(t1.type, tok::identifier);

    auto t2 = lex.next();
    EXPECT_EQ(t2.type, tok::identifier);
    EXPECT_EQ(t2.loc.line, 2);

    auto t3 = lex.next();
    EXPECT_EQ(t3.type, tok::eof);
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