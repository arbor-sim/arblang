#include <stdexcept>
#include <string>

#include <arblang/parser.hpp>
#include <arblang/pprintf.hpp>
#include <arblang/token.hpp>

namespace al {

parser::parser(const std::string& module): lexer(module.c_str()) {}

void parser::parse() {
    while (current().type != tok::eof) {
        switch (current().type) {
            case tok::module:
                modules_.push_back(parse_module());
                break;
            case tok::error:
                throw std::runtime_error(current().spelling);
            default:
                throw std::runtime_error(pprintf("Unexpected token '%'", current().spelling));
                break;
        }
    }
}

expr parser::parse_module() {
    module_expr m;
    auto t = next();
    if (t.type != tok::identifier) {
        throw std::runtime_error(pprintf("Unexpected token '%', expected identifier", t.spelling));
    }

    m.name = t.spelling;
    t = next();
    if (t.type != tok::lbrace) {
        throw std::runtime_error(pprintf("Unexpected token '%', expected '{'", t.spelling));
    }

    t = next();
    while (t.type != tok::rbrace) {
        switch (t.type) {
            case tok::parameter:
                m.constants.push_back(parse_parameter());
                break;
            case tok::constant:
                m.constants.push_back(parse_constant());
                break;
            case tok::record:
                m.constants.push_back(parse_record());
                break;
            case tok::function:
                m.constants.push_back(parse_function());
                break;
            case tok::import:
                m.constants.push_back(parse_import());
                break;
            default:
                throw std::runtime_error(pprintf("Unexpected token '%'", t.spelling));
                break;
        }
    }
    next();
    return make_expr<module_expr>(std::move(m));
}

expr parser::parse_parameter() {return nullptr;};
expr parser::parse_constant()  {return nullptr;};
expr parser::parse_record()    {return nullptr;};
expr parser::parse_function()  {return nullptr;};
expr parser::parse_import()    {return nullptr;};


} // namespace al