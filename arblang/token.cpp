#include <algorithm>
#include <optional>
#include <ostream>
#include <unordered_map>

#include <arblang/token.hpp>

namespace al {
std::ostream& operator<< (std::ostream& os, src_location const& loc) {
    return os << "location(line " << loc.line << ", col " << loc.column << ")";
}

std::unordered_map<std::string, tok> token::keyword_to_token = {
        {"if",          tok::if_stmt},
        {"else",        tok::else_stmt},
        {"min",         tok::min},
        {"max",         tok::max},
        {"exp",         tok::exp},
        {"sin",         tok::sin},
        {"cos",         tok::cos},
        {"log",         tok::log},
        {"abs",         tok::abs},
        {"exprelr",     tok::exprelr},
        {"module",      tok::module},
        {"parameter",   tok::parameter},
        {"constant",    tok::constant},
        {"record",      tok::record},
        {"function",    tok::function},
        {"import",      tok::import},
};

std::unordered_map<tok, std::string> token::token_to_string = {
        {tok::eof,        "eof"},
        {tok::eq,         "="},
        {tok::plus,       "+"},
        {tok::minus,      "-"},
        {tok::times,      "*"},
        {tok::divide,     "/"},
        {tok::pow,        "^"},
        {tok::lnot,       "!"},
        {tok::lt,         "<"},
        {tok::le,        "<="},
        {tok::gt,         ">"},
        {tok::ge,        ">="},
        {tok::equality,   "=="},
        {tok::ne,         "!="},
        {tok::land,       "&&"},
        {tok::lor,        "||"},
        {tok::arrow,      "<->"},
        {tok::semicolon,  ";"},
        {tok::comma,      ","},
        {tok::dot,        "."},
        {tok::prime,      "'"},
        {tok::lbrace,     "{"},
        {tok::rbrace,     "}"},
        {tok::lparen,     "("},
        {tok::rparen,     ")"},
        {tok::identifier, "identifier"},
        {tok::unit,       "unit"},
        {tok::real,       "real"},
        {tok::integer,    "integer"},
        {tok::if_stmt,    "if"},
        {tok::else_stmt,  "else"},
        {tok::min,        "min"},
        {tok::max,        "max"},
        {tok::exp,        "exp"},
        {tok::cos,        "cos"},
        {tok::sin,        "sin"},
        {tok::log,        "log"},
        {tok::abs,        "abs"},
        {tok::exprelr,    "exprelr"},
        {tok::module,     "module"},
        {tok::parameter,  "parameter"},
        {tok::constant,   "constant"},
        {tok::record,     "record"},
        {tok::function,   "function"},
        {tok::import,     "import"},
        {tok::error,      "error"},
};

std::optional<tok> token::is_keyword(const std::string& identifier) {
    auto pos = keyword_to_token.find(identifier);
    return pos!=keyword_to_token.end()? std::optional(pos->second): std::nullopt;
}

std::ostream& operator<<(std::ostream& os, const token& t) {
    auto pos = token::token_to_string.find(t.type);
    auto type_string = pos==token::token_to_string.end()? std::string("<unknown token>"): pos->second;
    return os << "token( type (" << type_string << "), spelling (" << t.spelling << "), " << t.loc << ")";
}
} // namespace al
