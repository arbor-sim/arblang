#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "arblang/location.hpp"

namespace al {
enum class tok {
    eof, // end of file

    // infix binary ops
    // = + - * && ||
    eq, plus, minus, times, divide, pow, land, lor,

    // comparison
    lnot,    // !   named logical not, to avoid clash with C++ not keyword
    lt,      // <
    lte,     // <=
    gt,      // >
    gte,     // >=
    equality,// ==
    ne,      // !=

    // <->
    arrow,

    // ; '
    semicolon, comma, prime,

    // { }
    lbrace, rbrace,

    // ( )
    lparen, rparen,

    // variable/function names
    identifier,

    // unit
    unit,

    // numbers
    real, integer,

    // logical keywords
    if_stmt, else_stmt,

    // prefix binary operators
    min, max,

    // unary operators
    exp, sin, cos, log, abs,
    exprelr, // equivalent to x/(exp(x)-1) with exprelr(0)=1

    // keywoards
    module, parameter, constant,
    record, function, import,

    // error
    error
};

struct token {
    // The spelling string contains the text of the token as it was written:
    //   type = tok::real       : spelling = "3.1415"  (e.g.)
    //   type = tok::identifier : spelling = "foo_bar" (e.g.)
    //   type = tok::plus       : spelling = "+"       (always)
    //   type = tok::if_else    : spelling = "if"      (always)
    std::string spelling;
    tok type;
    location loc;

    token(tok tok, std::string sp, location loc={0,0}): spelling(std::move(sp)), type(tok), loc(loc) {};
    token(): type(tok::error), loc({0,0}) {};
};

std::string token_string(tok);
tok identifier_token(const std::string&);
bool is_keyword(const token&);
std::ostream& operator<< (std::ostream&, const token&);

} // namespace al