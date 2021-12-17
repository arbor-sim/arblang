#include <algorithm>
#include <optional>
#include <ostream>
#include <sstream>
#include <unordered_map>

#include <arblang/token.hpp>

namespace al {
std::string src_location::to_string() const {
    return "(location " + std::to_string(line) + " " + std::to_string(column) + ")";
}
std::ostream& operator<< (std::ostream& os, const src_location& loc) {
    return os << "(location " << loc.line << " " << loc.column << ")";
}
bool operator==(const src_location& lhs, const src_location& rhs) {
    return (lhs.line == rhs.line) && (lhs.column == rhs.column);
}

std::unordered_map<std::string, tok> token::keyword_to_token = {
    {"if",            tok::if_stmt},
    {"then",          tok::then_stmt},
    {"else",          tok::else_stmt},
    {"min",           tok::min},
    {"max",           tok::max},
    {"exp",           tok::exp},
    {"sin",           tok::sin},
    {"cos",           tok::cos},
    {"log",           tok::log},
    {"abs",           tok::abs},
    {"exprelr",       tok::exprelr},
    {"module",        tok::module},
    {"mechanism",     tok::mechanism},
    {"junction",      tok::junction},
    {"point",         tok::point},
    {"parameter",     tok::parameter},
    {"constant",      tok::constant},
    {"state",         tok::state},
    {"record",        tok::record},
    {"function",      tok::function},
    {"import",        tok::import},
    {"effect",        tok::effect},
    {"evolve",        tok::evolve},
    {"initial",       tok::initial},
    {"export",        tok::param_export},
    {"density",       tok::density},
    {"bind",          tok::bind},
    {"as",            tok::function},
    {"let",           tok::let},
    {"with",          tok::with},
    {"real",          tok::real},
    {"length",        tok::length},
    {"mass",          tok::mass},
    {"time",          tok::time},
    {"current",       tok::current},
    {"amount",        tok::amount},
    {"temperature",   tok::temperature},
    {"charge",        tok::charge},
    {"frequency",     tok::frequency},
    {"voltage",       tok::voltage},
    {"resistance",    tok::resistance},
    {"conductance",   tok::conductance},
    {"capacitance",   tok::capacitance},
    {"force",         tok::force},
    {"energy",        tok::energy},
    {"power",         tok::power},
    {"area",          tok::area},
    {"volume",        tok::volume},
    {"concentration", tok::concentration},
    {"membrane_potential",     tok::membrane_potential},
    {"current_density",        tok::current_density},
    {"molar_flux",             tok::molar_flux},
    {"internal_concentration", tok::internal_concentration},
    {"external_concentration", tok::external_concentration},
    {"nernst_potential",       tok::nernst_potential},
    {"molar_flow_rate",        tok::molar_flow_rate},
    {"internal_concentration_rate", tok::internal_concentration_rate},
    {"external_concentration_rate", tok::external_concentration_rate},
};

std::unordered_map<tok, std::string> token::token_to_string = {
    {tok::eof,           "eof"},
    {tok::eq,            "="},
    {tok::plus,          "+"},
    {tok::minus,         "-"},
    {tok::times,         "*"},
    {tok::divide,        "/"},
    {tok::pow,           "^"},
    {tok::lnot,          "!"},
    {tok::lt,            "<"},
    {tok::le,            "<="},
    {tok::gt,            ">"},
    {tok::ge,            ">="},
    {tok::equality,      "=="},
    {tok::ne,            "!="},
    {tok::land,          "&&"},
    {tok::lor,           "||"},
    {tok::arrow,         "<->"},
    {tok::semicolon,     ";"},
    {tok::colon,         ":"},
    {tok::comma,         ","},
    {tok::quote,         "\""},
    {tok::dot,           "."},
    {tok::lbrace,        "{"},
    {tok::rbrace,        "}"},
    {tok::lparen,        "("},
    {tok::rparen,        ")"},
    {tok::lbracket,      "["},
    {tok::rbracket,      "]"},
    {tok::identifier,    "identifier"},
    {tok::unit,          "unit"},
    {tok::floatpt,       "float"},
    {tok::integer,       "integer"},
    {tok::if_stmt,       "if"},
    {tok::then_stmt,     "then"},
    {tok::else_stmt,     "else"},
    {tok::min,           "min"},
    {tok::max,           "max"},
    {tok::exp,           "exp"},
    {tok::cos,           "cos"},
    {tok::sin,           "sin"},
    {tok::log,           "log"},
    {tok::abs,           "abs"},
    {tok::exprelr,       "exprelr"},
    {tok::mechanism,     "mechanism"},
    {tok::point,         "mechanism"},
    {tok::junction,      "junction"},
    {tok::module,        "module"},
    {tok::parameter,     "parameter"},
    {tok::constant,      "constant"},
    {tok::state,         "state"},
    {tok::record,        "record"},
    {tok::function,      "function"},
    {tok::import,        "import"},
    {tok::as,            "as"},
    {tok::let,           "let"},
    {tok::with,          "with"},
    {tok::effect,        "effect"},
    {tok::evolve,        "evolve"},
    {tok::initial,       "initial"},
    {tok::param_export,  "export"},
    {tok::density,       "density"},
    {tok::bind,          "bind"},
    {tok::ret,           ":"},
    {tok::real,          "real"},
    {tok::length,        "length"},
    {tok::mass,          "mass"},
    {tok::time,          "time"},
    {tok::current,       "current"},
    {tok::amount,        "amount"},
    {tok::temperature,   "temperature"},
    {tok::charge,        "charge"},
    {tok::frequency,     "frequency"},
    {tok::voltage,       "voltage"},
    {tok::resistance,    "resistance"},
    {tok::conductance,   "conductance"},
    {tok::capacitance,   "capacitance"},
    {tok::force,         "force"},
    {tok::energy,        "energy"},
    {tok::power,         "power"},
    {tok::area,          "area"},
    {tok::volume,        "volume"},
    {tok::concentration, "concentration"},
    {tok::error,         "error"},
    {tok::membrane_potential,     "membrane_potential"},
    {tok::current_density,        "current_density"},
    {tok::molar_flux,             "molar_flux"},
    {tok::internal_concentration, "internal_concentration"},
    {tok::external_concentration, "external_concentration"},
    {tok::nernst_potential,       "nernst_potential"},
    {tok::molar_flow_rate,        "molar_flow_rate"},
    {tok::internal_concentration_rate, "internal_concentration_rate"},
    {tok::external_concentration_rate, "external_concentration_rate"},
};

std::unordered_map<tok, int> token::binop_prec = {
    {tok::lor,      2},
    {tok::land,     3},
    {tok::equality, 4},
    {tok::ne,       4},
    {tok::lt,       5},
    {tok::le,       5},
    {tok::gt,       5},
    {tok::ge,       5},
    {tok::plus,     6},
    {tok::minus,    6},
    {tok::times,    7},
    {tok::divide,   7},
    {tok::pow,      8},
    {tok::dot,      9},
};

int token::precedence() const {
    if(!binop_prec.count(type)) return -1;
    return binop_prec.find(type)->second;
}

bool token::right_associative() const {
    return type==tok::pow;
}

bool token::quantity() const {
    switch (type) {
        case tok::real:
        case tok::length:
        case tok::mass:
        case tok::time:
        case tok::current:
        case tok::amount:
        case tok::temperature:
        case tok::charge:
        case tok::frequency:
        case tok::voltage:
        case tok::resistance:
        case tok::conductance:
        case tok::capacitance:
        case tok::force:
        case tok::energy:
        case tok::power:
        case tok::area:
        case tok::volume:
        case tok::concentration:
        case tok::error:
            return true;
        default: return false;
    }
}

bool token::mechanism_kind() const {
    switch (type) {
        case tok::density:
        case tok::concentration:
        case tok::point:
        case tok::junction:
            return true;
        default: return false;
    }
}


bool token::bindable() const {
    switch (type) {
        case tok::membrane_potential:
        case tok::temperature:
        case tok::current_density:
        case tok::molar_flux:
        case tok::charge:
        case tok::internal_concentration:
        case tok::external_concentration:
        case tok::nernst_potential:
            return true;
        default: return false;
    }
}

bool token::affectable() const {
    switch (type) {
        case tok::current_density:
        case tok::current:
        case tok::molar_flux:
        case tok::molar_flow_rate:
        case tok::internal_concentration_rate:
        case tok::external_concentration_rate:
            return true;
        default: return false;
    }
}

bool token::ion_bindable() const {
    switch (type) {
        case tok::molar_flux:
        case tok::current_density:
        case tok::charge:
        case tok::current:
        case tok::internal_concentration:
        case tok::external_concentration:
        case tok::nernst_potential:
        case tok::molar_flow_rate:
        case tok::internal_concentration_rate:
        case tok::external_concentration_rate:
            return true;
        default: return false;
    }
}

std::optional<tok> token::tokenize(const std::string& identifier) {
    auto pos = keyword_to_token.find(identifier);
    return pos!=keyword_to_token.end()? std::optional(pos->second): std::nullopt;
}

std::ostream& operator<<(std::ostream& os, const token& t) {
    auto pos = token::token_to_string.find(t.type);
    auto type_string = pos==token::token_to_string.end()? std::string("<unknown token>"): pos->second;
    return os << "token( type (" << type_string << "), spelling (" << t.spelling << "), " << t.loc << ")";
}
} // namespace al
