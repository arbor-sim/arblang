#include <optional>
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
        }
    }
    next();
    return make_expr<module_expr>(std::move(m));
}

expr parser::parse_parameter() {
    auto loc = current().loc;

    auto iden = parse_identifier();

    auto t = current();
    if (t.type != tok::eq) {
        throw std::runtime_error("Expected e=, got " + t.spelling);
    }
    next(); // consume '='

    auto value = parse_expr();

    return make_expr<parameter_expr>(std::move(iden), std::move(value), loc);
};
expr parser::parse_constant()  {return nullptr;};
expr parser::parse_record()    {return nullptr;};
expr parser::parse_function()  {return nullptr;};
expr parser::parse_import()    {return nullptr;};

expr parser::parse_call() {
    if (current().type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + current().spelling);
    }
    auto iden = current();
    auto t = next(); // consume function name

    if (t.type != tok::lparen) {
        throw std::runtime_error("Expected '(', got " + t.spelling );
    }
    t = next(); // consume '('

    std::vector<expr> args;
    while (t.type != tok::rparen) {
        args.emplace_back(parse_expr());
        if (t.type == tok::rparen) break;

        // Check for comma between arguments
        if (t.type != tok::comma) {
            throw std::runtime_error("Expected ',' between function arguments, got " + t.spelling );
        }
        t = next(); // consume ','
    }

    if (t.type != tok::rparen) {
        throw std::runtime_error("Expected ')', got " + t.spelling );
    }
    next(); // consume ')'

    return make_expr<call_expr>(iden.spelling, std::move(args), iden.loc);
}

expr parser::parse_field() {
    if (current().type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + current().spelling);
    }
    auto iden = current();
    auto t = next(); // consume record name

    if (t.type != tok::dot) {
        throw std::runtime_error("Expected '.', got " + t.spelling );
    }
    auto field = next(); // consume '.'

    if (field.type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + field.spelling);
    }
    next(); // consume field name

    return make_expr<field_expr>(iden.spelling, field.spelling, iden.loc);
}
expr parser::parse_let() {
    if (current().type != tok::let) {
        throw std::runtime_error("Expected 'let', got " + current().spelling);
    }
    auto let = current();
    auto t = next(); // consume 'let'

    auto iden = parse_identifier();

    t = current();
    if (t.type != tok::eq) {
        throw std::runtime_error("Expected '=', got " + t.spelling);
    }
    t = next(); // consume '='

    auto value = parse_expr();

    bool expect_rbrace = false;
    if (t.type == tok::lbrace) {
        expect_rbrace = true;
        next(); // consume '{'
    }

    auto body = parse_expr();

    if (expect_rbrace && (current().type != tok::rbrace)) {
        throw std::runtime_error("Expected '}', got " + t.spelling);
    }
    next(); // consume '}'

    return make_expr<let_expr>(std::move(iden), std::move(value), std::move(body), let.loc);
}

expr parser::parse_conditional() {
    if (current().type != tok::if_stmt) {
        throw std::runtime_error("Expected 'if', got " + current().spelling);
    }
    auto if_stmt = current();
    auto t = next(); // consume 'if'

    if (t.type != tok::lparen) {
        throw std::runtime_error("Expected '(' expression, got " + t.spelling);
    }
    t = next(); // consume '('

    auto condition = parse_expr();
    if (auto binary = std::get_if<binary_expr>(condition.get())) {
        if (!binary->is_boolean()) {
            throw std::runtime_error("Expected boolean expression");
        }
    } else if (auto unary = std::get_if<unary_expr>(condition.get())) {
        if (!unary->is_boolean()) {
            throw std::runtime_error("Expected boolean expression");
        }
    } else {
        throw std::runtime_error("Expected boolean expression");
    };

    if (t.type != tok::rparen) {
        throw std::runtime_error("Expected ')' expression, got " + t.spelling);
    }
    t = next(); // consume ')'

    if (t.type != tok::lbrace) {
        throw std::runtime_error("Expected '{' expression, got " + t.spelling);
    }
    t = next(); // consume '{'

    auto if_true = parse_expr();

    if (t.type != tok::rbrace) {
        throw std::runtime_error("Expected '}' expression, got " + t.spelling);
    }
    t = next(); // consume '}'

    if (t.type != tok::else_stmt) {
        throw std::runtime_error("Expected 'else', got " + current().spelling);
    }
    t = next(); // consume 'else'

    if (t.type != tok::lbrace) {
        throw std::runtime_error("Expected '{' expression, got " + t.spelling);
    }
    t = next(); // consume '{'

    auto if_false = parse_expr();

    if (t.type != tok::rbrace) {
        throw std::runtime_error("Expected '}' expression, got " + t.spelling);
    }
    t = next(); // consume '}'

    return make_expr<conditional_expr>(std::move(condition), std::move(if_true), std::move(if_false), if_stmt.loc);
}

expr parser::parse_float() {
    if (current().type != tok::real) {
        throw std::runtime_error("Expected floating point number, got " + current().spelling);
    }
    auto num = current();
    auto t = next(); // consume float

    std::string unit;
    if (t.type == tok::identifier) {
        unit = t.spelling;
        next();
    }
    return make_expr<float_expr>(std::stold(num.spelling), unit, num.loc);
}

expr parser::parse_int() {
    if (current().type != tok::integer) {
        throw std::runtime_error("Expected integer number, got " + current().spelling);
    }
    auto num = current();
    auto t = next(); // consume int

    std::string unit;
    if (t.type == tok::identifier) {
        unit = t.spelling;
        next();
    }
    return make_expr<int_expr>(std::stoll(num.spelling), unit, num.loc);
}

expr parser::parse_prefix() {
    auto parse_unary_paren_expr = [&]() {
        if (current().type != tok::lparen) {
            throw std::runtime_error("Expected '(' expression, got " + current().spelling);
        }
        next(); // consume '('
        auto e = parse_expr();
        if (current().type != tok::rparen) {
            throw std::runtime_error("Expected ')' expression, got " + current().spelling);
        }
        next(); // consume ')'
        return e;
    };
    auto parse_binary_paren_expr = [&]() {
        if (current().type != tok::lparen) {
            throw std::runtime_error("Expected '(' expression, got " + current().spelling);
        }
        next(); // consume '('

        auto lhs = parse_expr();

        if (current().type != tok::comma) {
            throw std::runtime_error("Expected ',' expression, got " + current().spelling);
        }
        next(); // consume ','

        auto rhs = parse_expr();

        if (current().type != tok::rparen) {
            throw std::runtime_error("Expected ')' expression, got " + current().spelling);
        }
        next(); // consume ')'
        return std::make_pair(lhs, rhs);
    };

    auto prefix_op = current();
    switch (prefix_op.type) {
        case tok::exp:
            return make_expr<unary_expr>(unary_op::exp, parse_unary_paren_expr(), prefix_op.loc);
        case tok::exprelr:
            return make_expr<unary_expr>(unary_op::exprelr, parse_unary_paren_expr(), prefix_op.loc);
        case tok::log:
            return make_expr<unary_expr>(unary_op::log, parse_unary_paren_expr(), prefix_op.loc);
        case tok::cos:
            return make_expr<unary_expr>(unary_op::cos, parse_unary_paren_expr(), prefix_op.loc);
        case tok::sin:
            return make_expr<unary_expr>(unary_op::sin, parse_unary_paren_expr(), prefix_op.loc);
        case tok::abs:
            return make_expr<unary_expr>(unary_op::abs, parse_unary_paren_expr(), prefix_op.loc);
        case tok::lnot:
            return make_expr<unary_expr>(unary_op::lnot, parse_expr(), prefix_op.loc);
        case tok::minus:
            return make_expr<unary_expr>(unary_op::neg, parse_expr(), prefix_op.loc);
        case tok::plus:
            return parse_expr();
        case tok::max: {
            auto pair = parse_binary_paren_expr();
            return make_expr<binary_expr>(binary_op::max, std::move(pair.first), std::move(pair.second), prefix_op.loc);
        }
        case tok::min: {
            auto pair = parse_binary_paren_expr();
            return make_expr<binary_expr>(binary_op::min, std::move(pair.first), std::move(pair.second), prefix_op.loc);
        }
        default:
            throw std::runtime_error("expected prefix operator, got " + prefix_op.spelling);
    }
}

expr parser::parse_infix() {

}

expr parser::parse_identifier() {

}

// This is one of:
// - call_expr
// - field_expr
// - let_expr
// - conditional_expr
// - identifier_expr
// - float_expr
// - int_expr
// - unary_expr
// - binary_expr
expr parser::parse_expr() {

}

type_expr parser::parse_type() {}
} // namespace al