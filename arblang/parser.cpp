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
    m.loc = t.loc;
    t = next(); // consume name

    if (t.type != tok::lbrace) {
        throw std::runtime_error(pprintf("Unexpected token '%', expected '{'", t.spelling));
    }
    t = next(); // consume '{'

    while (t.type != tok::rbrace) {
        switch (t.type) {
            case tok::parameter:
                m.parameters.push_back(parse_parameter());
                break;
            case tok::constant:
                m.constants.push_back(parse_constant());
                break;
            case tok::record:
                m.records.push_back(parse_record());
                break;
            case tok::function:
                m.functions.push_back(parse_function());
                break;
            case tok::import:
                m.imports.push_back(parse_import());
                break;
            default:
                throw std::runtime_error(pprintf("Unexpected token '%'", t.spelling));
        }
        t = current();
    }
    if (t.type != tok::rbrace) {
        throw std::runtime_error(pprintf("Unexpected token '%', expected '}'", t.spelling));
    }
    next(); // consume '}'

    return make_expr<module_expr>(std::move(m));
}

expr parser::parse_parameter() {
    if (current().type != tok::parameter) {
        throw std::runtime_error("Expected parameter, got " + current().spelling);
    }
    auto loc = current().loc;
    next(); // consume 'parameter'

    auto iden = parse_identifier();

    auto t = current();
    if (t.type != tok::eq) {
        throw std::runtime_error("Expected =, got " + t.spelling);
    }
    next(); // consume '='

    auto value = parse_expr();

    if (current().type != tok::semicolon) {
        throw std::runtime_error("Expected ;, got " + current().spelling);
    }
    next(); // consume ';'

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
    next(); // consume '}'

    return make_expr<conditional_expr>(std::move(condition), std::move(if_true), std::move(if_false), if_stmt.loc);
}

expr parser::parse_float() {
    if (current().type != tok::floatpt) {
        throw std::runtime_error("Expected floating point number, got " + current().spelling);
    }
    auto num = current();
    auto t = next(); // consume float

    // not enough!
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

expr parser::parse_identifier() {
    auto type = parse_type();
    if (current().type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + current().spelling);
    }
    auto iden = current();
    next(); // cosume identifier
    return make_expr<identifier_expr>(type, iden.spelling, iden.loc);
}

expr parser::parse_prefix() {
    auto prefix_op = current();
    switch (prefix_op.type) {
        case tok::exp:
        case tok::exprelr:
        case tok::log:
        case tok::cos:
        case tok::sin:
        case tok::abs:
        case tok::lnot: {
            if (current().type != tok::lparen) {
                throw std::runtime_error("Expected '(' expression, got " + current().spelling);
            }
            next(); // consume '('
            auto e = parse_expr();
            if (current().type != tok::rparen) {
                throw std::runtime_error("Expected ')' expression, got " + current().spelling);
            }
            next(); // consume ')'
            return make_expr<unary_expr>(prefix_op.type, std::move(e), prefix_op.loc);
        }
        case tok::minus:
            next(); // consume '-'
            return make_expr<unary_expr>(prefix_op.type, parse_expr(), prefix_op.loc);
        case tok::plus:
            next(); // consume '+'
            return parse_expr();
        case tok::max:
        case tok::min: {
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
            return make_expr<binary_expr>(prefix_op.type, std::move(lhs), std::move(rhs), prefix_op.loc);
        }
        default: throw std::runtime_error("expected prefix operator, got " + prefix_op.spelling);
    }
}

// This is one of:
// - call_expr
// - field_expr
// - identifier_expr
// - let_expr
// - conditional_expr
// - float_expr
// - int_expr
// - prefix_expr
expr parser::parse_unary() {
    auto t = current();
    switch (t.type) {
        case tok::lparen: {
            t = next(); // consume '('
            auto e = parse_expr();
            if (t.type != tok::rparen) {
                throw std::runtime_error("Expected ')' expression, got " + t.spelling);
            }
            next(); // consume ')'
            return e;
        }
        case tok::identifier:
            if (peek().type == tok::lparen) {
                return parse_call();
            }
            if (peek().type == tok::dot) {
                return parse_field();
            }
            return parse_identifier();
        case tok::let:
            return parse_let();
        case tok::if_stmt:
            return parse_conditional();
        case tok::floatpt:
            return parse_float();
        case tok::integer:
            return parse_int();
        default:
            return parse_prefix();
    }
}

// Handles infix operations
expr parser::parse_binary(expr&& lhs, const token& lop) {
    auto lop_prec = lop.precedence();
    auto rhs = parse_expr(lop_prec);

    auto rop = current();
    auto rop_prec = rop.precedence();

    if (rop_prec > lop_prec) {
        throw std::runtime_error("parse_binop() : encountered operator of higher precedence");
    }
    if (rop_prec < lop_prec) {
        return make_expr<binary_expr>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
    }
    next(); // consume rop
    if (rop.right_associative()) {
        rhs = parse_binary(std::move(rhs), rop);
        return make_expr<binary_expr>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
    }
    else {
        lhs = make_expr<binary_expr>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
        return parse_binary(std::move(lhs), rop);
    }
}

// Handles full nested expressions
expr parser::parse_expr(int prec) {
    auto lhs = parse_unary();

    // Combine all sub-expressions with precedence greater than prec.
    for (;;) {
        auto op = current();
        auto op_prec = op.precedence();

        // Note: all tokens that are not infix binary operators have a precedence of -1,
        // so expressions like function calls will short circuit this loop here.
        if (op_prec <= prec) return lhs;

        next(); // consume the infix binary operator

        lhs = parse_binary(std::move(lhs), op);
    }
}

// Handles infix type operations
t_expr parser::parse_binary_type(t_expr&& lhs, const token& lop) {
    auto lop_prec = lop.precedence();
    auto rhs = parse_type(lop_prec);

    auto rop = current();
    auto rop_prec = rop.precedence();

    if (rop_prec > lop_prec) {
        throw std::runtime_error("parse_binop() : encountered operator of higher precedence");
    }
    if (rop_prec < lop_prec) {
        return make_t_expr<quantity_binary_type>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
    }
    next(); // consume rop
    if (rop.right_associative()) {
        rhs = parse_binary_type(std::move(rhs), rop);
        return make_t_expr<quantity_binary_type>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
    }
    else {
        lhs = make_t_expr<quantity_binary_type>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
        return parse_binary_type(std::move(lhs), rop);
    }
}


t_expr parser::parse_type(int prec) {
    t_expr lhs;
    if (current().quantity()) {
        lhs = make_t_expr<quantity_type>(current().type, current().loc);
    }
    else if (current().type == tok::integer) {
        lhs = make_t_expr<integer_type>(std::stoll(current().spelling), current().loc);
    }
    else {
        throw std::runtime_error("Expected quantity or integer token, got " + current().spelling);
    }
    next(); // consume 'quantity'

    // Combine all sub-expressions with precedence greater than prec.
    for (;;) {
        auto op = current();
        auto op_prec = op.precedence();

        // Note: all tokens that are not infix binary operators have a precedence of -1,
        // so expressions like function calls will short circuit this loop here.
        if (op_prec <= prec) return lhs;

        next(); // consume the infix binary operator

        lhs = parse_binary_type(std::move(lhs), op);
    }
}
} // namespace al