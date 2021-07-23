#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace al {

    struct src_location {
    int line;
    int column;

    src_location(): src_location(1, 1) {}
    src_location(int ln, int col): line(ln), column(col) {}
};

std::ostream& operator<< (std::ostream& os, src_location const& loc);

enum class tok {
    eof, // end of file

    // infix binary ops
    // = + - * && ||
    eq, plus, minus, times, divide, pow, land, lor,

    // comparison
    lnot,    // !   named logical not, to avoid clash with C++ not keyword
    lt,      // <
    le,      // <=
    gt,      // >
    ge,      // >=
    equality,// ==
    ne,      // !=

    // <->
    arrow,

    // ; '
    semicolon, comma, dot, prime,

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
    let, as, ret,

    // error
    error
};

struct token {
    // The spelling string contains the text of the token as it was written:
    //   type = tok::real       : spelling = "3.1415"  (e.g.)
    //   type = tok::identifier : spelling = "foo_bar" (e.g.)
    //   type = tok::plus       : spelling = "+"       (always)
    //   type = tok::if_else    : spelling = "if"      (always)
    src_location loc;
    tok type;
    std::string spelling;

    friend std::ostream& operator<< (std::ostream&, const token&);
    static std::optional<tok> is_keyword(const std::string&);
    int precedence() const;
    bool right_associative() const;

private:
    static std::unordered_map<tok, int> binop_prec;
    static std::unordered_map<std::string, tok> keyword_to_token;
    static std::unordered_map<tok, std::string> token_to_string;
};

std::ostream& operator<< (std::ostream&, const token&);

} // namespace al