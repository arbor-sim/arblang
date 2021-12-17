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
    std::string to_string() const;
};

std::ostream& operator<< (std::ostream& os, const src_location& loc);
std::string to_string(const src_location& loc);

bool operator==(const src_location& lhs, const src_location& rhs);

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

    // ; : , .
    semicolon, colon, comma, dot,

    // { }
    lbrace, rbrace,

    // ( )
    lparen, rparen,

    // [ ]
    lbracket, rbracket,

    // variable/function names
    identifier,

    // any string between quotes
    quoted,

    // unit
    unit,

    // numbers
    floatpt, integer,

    // logical keywords
    if_stmt, then_stmt, else_stmt,

    // prefix binary operators
    min, max,

    // unary operators
    exp, sin, cos, log, abs,
    exprelr, // equivalent to x/(exp(x)-1) with exprelr(0)=1

    // keywords
    mechanism,
    point, junction, // density, concentration already exist
    module,
    parameter, constant, state,
    record, function, import,
    with, let, as, ret,
    effect, evolve, initial,
    bind, param_export, density,

    // quantity keywords
    real, length, mass, time, current,
    amount, temperature, charge, frequency, voltage,
    resistance, conductance, capacitance, force,
    energy, power, area, volume, concentration,

    // bindable keywords
    membrane_potential, // temperature already exists
    current_density, molar_flux, //charge already exists
    internal_concentration, external_concentration,
    nernst_potential,

    // affectable keywords
    molar_flow_rate, internal_concentration_rate,
    external_concentration_rate,

    // error
    error
};

struct token {
    // The spelling string contains the text of the token as it was written:
    //   type = tok::floatpt    : spelling = "3.1415"  (e.g.)
    //   type = tok::identifier : spelling = "foo_bar" (e.g.)
    //   type = tok::plus       : spelling = "+"       (always)
    //   type = tok::if_else    : spelling = "if"      (always)
    src_location loc;
    tok type;
    std::string spelling;

    static std::optional<tok> tokenize(const std::string&);
    bool quantity() const;
    bool mechanism_kind() const;
    bool bindable() const;
    bool affectable() const;
    bool ion_bindable() const;
    bool right_associative() const;
    int precedence() const;

    friend std::ostream& operator<< (std::ostream&, const token&);

private:
    static std::unordered_map<tok, int> binop_prec;
    static std::unordered_map<std::string, tok> keyword_to_token;
    static std::unordered_map<tok, std::string> token_to_string;
};

std::ostream& operator<< (std::ostream&, const token&);

} // namespace al