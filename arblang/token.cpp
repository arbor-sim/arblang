#include <algorithm>
#include <mutex>
#include <unordered_map>

#include <arblang/token.hpp>

namespace al {
static std::unordered_map<std::string, tok> keyword_map = {
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

static std::unordered_map<tok, std::string> token_map = {
        {tok::eof,        "eof"},
        {tok::eq,         "="},
        {tok::plus,       "+"},
        {tok::minus,      "-"},
        {tok::times,      "*"},
        {tok::divide,     "/"},
        {tok::pow,        "^"},
        {tok::lnot,       "!"},
        {tok::lt,         "<"},
        {tok::lte,        "<="},
        {tok::gt,         ">"},
        {tok::gte,        ">="},
        {tok::equality,   "=="},
        {tok::ne,         "!="},
        {tok::land,       "&&"},
        {tok::lor,        "||"},
        {tok::arrow,      "<->"},
        {tok::semicolon,  ";"},
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

bool is_keyword(const token& t) {
    return std::any_of(keyword_map.begin(), keyword_map.end(), [&](const auto&& el){return el.second == t.type;});
}

tok identifier_token(const std::string& identifier) {
    auto pos = keyword_map.find(identifier);
    return pos==keyword_map.end()? tok::identifier: pos->second;
}

std::string token_string(tok token) {
    auto pos = token_map.find(token);
    return pos==token_map.end()? std::string("<unknown token>"): pos->second;
}

std::ostream& operator<<(std::ostream& os, const token& t) {
    return os << "token( type (" << token_string(t.type) << "), spelling (" << t.spelling << "), " << t.loc << ")";
}
} // namespace al
