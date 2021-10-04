#include <optional>
#include <stdexcept>
#include <string>

#define FMT_HEADER_ONLY YES
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/compile.h>

#include <arblang/parser.hpp>
#include <arblang/token.hpp>

namespace al {

parser::parser(const std::string& module): lexer(module.c_str()) {}

void parser::parse() {
    auto t = current();
    while (t.type != tok::eof) {
        switch (t.type) {
            case tok::module:
                modules_.push_back(parse_module());
                break;
            case tok::error:
                throw std::runtime_error(t.spelling);
            default:
                throw std::runtime_error(fmt::format("Unexpected token {}", t.spelling));
        }
        t = current();
    }
}

expr parser::parse_module() {
    module_expr m;
    auto t = current();
    if (t.type != tok::module) {
        throw std::runtime_error(fmt::format("Unexpected token '{}', expected identifier", t.spelling));
    }
    t = next(); // consume module

    if (t.type != tok::identifier) {
        throw std::runtime_error(fmt::format("Unexpected token '{}', expected identifier", t.spelling));
    }
    m.name = t.spelling;
    m.loc = t.loc;
    t = next(); // consume name

    if (t.type != tok::lbrace) {
        throw std::runtime_error(fmt::format("Unexpected token '{}', expected '{'", t.spelling));
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
                m.records.push_back(parse_record_alias());
                break;
            case tok::function:
                m.functions.push_back(parse_function());
                break;
            case tok::import:
                m.imports.push_back(parse_import());
                break;
            default:
                throw std::runtime_error(fmt::format("Unexpected token '{}'", t.spelling));
        }
        t = current();
    }
    if (t.type != tok::rbrace) {
        throw std::runtime_error(fmt::format("Unexpected token '{}', expected '}'", t.spelling));
    }
    next(); // consume '}'

    return make_expr<module_expr>(std::move(m));
}

// A parameter declaration of the form:
// parameter `iden` [: `type`] = `value_expression`;
expr parser::parse_parameter() {
    auto t = current();
    if (t.type != tok::parameter) {
        throw std::runtime_error("Expected parameter, got " + t.spelling);
    }
    auto loc = t.loc;
    next(); // consume 'parameter'

    auto iden = parse_typed_identifier();

    t = current();
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

// A constant declaration of the form:
// constant `iden` [: `type`] = `value_expression`;
expr parser::parse_constant()  {
    auto t = current();
    if (t.type != tok::constant) {
        throw std::runtime_error("Expected constant, got " + t.spelling);
    }
    auto loc = t.loc;
    next(); // consume 'constant'

    auto iden = parse_typed_identifier();

    t = current();
    if (t.type != tok::eq) {
        throw std::runtime_error("Expected =, got " + t.spelling);
    }
    next(); // consume '='

    auto value = parse_expr();

    if (current().type != tok::semicolon) {
        throw std::runtime_error("Expected ;, got " + current().spelling);
    }
    next(); // consume ';'

    return make_expr<constant_expr>(std::move(iden), std::move(value), loc);
};

// A record definition (type alias) of the form:
// record `iden` {`field_id0`: `type0`, `field_id1`: `type_1`, ...}[;]
expr parser::parse_record_alias()    {
    auto t = current();
    if (t.type != tok::record) {
        throw std::runtime_error("Expected record, got " + t.spelling);
    }
    auto loc = t.loc;
    t = next(); // consume 'record'

    if (t.type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + t.spelling);
    }
    auto iden = t.spelling;
    next(); // consume identifier

    auto type = parse_record_type();

    if (t.type == tok::semicolon) {
        next(); // consume ;
    }

    return make_expr<record_alias_expr>(iden, type, loc);
};

// A function definition of the form:
// function `iden` (`arg0`:`type0`, `arg1`:`type`, ...) [: `return_type`] {`value_expression`}[;]
expr parser::parse_function() {
    auto t = current();
    if (t.type != tok::function) {
        throw std::runtime_error("Expected function, got " + t.spelling);
    }
    auto loc = t.loc;
    t = next(); // consume 'function'

    if (t.type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + t.spelling);
    }
    auto name = t.spelling;
    t = next(); // consume identifier

    if (t.type != tok::lparen) {
        throw std::runtime_error("Expected (, got " + t.spelling);
    }
    t = next(); // consume '('

    std::vector<expr> args;
    while (t.type != tok::rparen) {
        if (peek().type != tok::colon) {
            throw std::runtime_error("function arguments must have a type.");
        }
        args.push_back(parse_typed_identifier());
        t = current();
        if (t.type == tok::rparen) break;
        if (t.type != tok::comma) {
            throw std::runtime_error("Expected `,`, got " + t.spelling);
        }
        t = next(); // consume ','
    }

    if (t.type != tok::rparen) {
        throw std::runtime_error("Expected ), got " + t.spelling);
    }
    t = next(); // consume ')'

    std::optional<t_expr> ret_quantity = {};
    if (t.type == tok::colon) {
        next(); // consume :
        ret_quantity = parse_type();
    }

    t = current();
    if (t.type != tok::lbrace) {
        throw std::runtime_error("Expected {, got " + t.spelling);
    }
    next(); // consume '{'

    auto ret_value = parse_expr();

    t = current();
    if (t.type != tok::rbrace) {
        throw std::runtime_error("Expected }, got " + t.spelling);
    }
    t = next(); // consume '}'

    if (t.type == tok::semicolon) {
        next(); // consume ;
    }

    return make_expr<function_expr>(name, args, ret_quantity, ret_value, loc);
};

// A module import of the form:
// import `iden` [as `alias`][;]
expr parser::parse_import() {
    auto t = current();
    if (t.type != tok::import) {
        throw std::runtime_error("Expected import, got " + t.spelling);
    }
    auto loc = current().loc;
    t = next(); // consume 'import'

    if (t.type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + t.spelling);
    }
    auto module_name = t.spelling;
    t = next(); // consume module name

    auto module_alias = module_name;
    if (t.type == tok::as) {
        t = next(); //consume as

        if (t.type != tok::identifier) {
            throw std::runtime_error("Expected identifier, got " + t.spelling);
        }
        module_alias = t.spelling;
        next(); // consume module alias
    }
    if (t.type == tok::semicolon) {
        next(); // consume ;
    }
    return make_expr<import_expr>(module_name, module_alias, loc);
};

// A function call of the form:
// `func_name`(`value_expression0`, `value_exprssion1`, ...)
expr parser::parse_call() {
    auto t = current();
    if (t.type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + t.spelling);
    }
    auto iden = t.spelling;
    auto loc = t.loc;
    t = next(); // consume function name

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

    return make_expr<call_expr>(iden, std::move(args), loc);
}

// An object of the form:
// [`name`] {`field0`[:`type0`] = `value_expression0`; `field1`[:`type1`] = `value_expression1`}
expr parser::parse_object() {
    auto t = current();
    auto loc = t.loc;
    std::optional<std::string> record_name;
    if (t.type == tok::identifier) {
        record_name = t.spelling;
        t = next(); // consume record name
    }

    if (t.type != tok::lbrace) {
        throw std::runtime_error("Expected '{', got " + t.spelling );
    }
    t = next(); // consume '{'

    std::vector<expr> fields, values;
    while (t.type != tok::rbrace) {
        fields.emplace_back(parse_typed_identifier());
        t = current();
        if (t.type != tok::eq) {
            throw std::runtime_error("Expected '=', got " + t.spelling );
        }
        next(); // consume '='
        values.emplace_back(parse_expr());

        // Check for semicolon between field assignments
        t = current();
        if (t.type != tok::semicolon) {
            throw std::runtime_error("Expected ';' between record fields, got " + t.spelling );
        }
        t = next(); // consume ';'
    }

    if (t.type != tok::rbrace) {
        throw std::runtime_error("Expected '}', got " + t.spelling );
    }
    next(); // consume '}'

    return make_expr<object_expr>(record_name, std::move(fields), std::move(values), loc);
}

// A let expression of the form:
// let `iden`[: `type`] = `value_expression0`; `value_expression1`
expr parser::parse_let() {
    if (current().type != tok::let) {
        throw std::runtime_error("Expected 'let', got " + current().spelling);
    }
    auto let = current();
    next(); // consume 'let'

    auto iden = parse_typed_identifier();

    auto t = current();
    if (t.type != tok::eq) {
        throw std::runtime_error("Expected '=', got " + t.spelling);
    }
    next(); // consume '='

    auto value = parse_expr();

    t = current();
    if (t.type != tok::semicolon) {
        throw std::runtime_error("Expected ';', got " + t.spelling);
    }
    next(); // consume ';'
    auto body = parse_expr();
    return make_expr<let_expr>(std::move(iden), std::move(value), std::move(body), let.loc);
}

// A with expression of the form:
// with `iden`; `value_expression`
expr parser::parse_with() {
    if (current().type != tok::with) {
        throw std::runtime_error("Expected 'with', got " + current().spelling);
    }
    auto with = current();
    auto t = next(); // consume 'with'

    if (t.type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + current().spelling);
    }
    auto iden = t.spelling;
    t = next(); // consume identifier

    if (t.type != tok::semicolon) {
        throw std::runtime_error("Expected ';', got " + t.spelling);
    }
    next(); // consume ';'

    auto body = parse_expr();

    return make_expr<with_expr>(std::move(iden), std::move(body), with.loc);
}

// A conditional expression of the form:
// if `boolean_expression` then `value_expression0` else `value_expression1`
expr parser::parse_conditional() {
    if (current().type != tok::if_stmt) {
        throw std::runtime_error("Expected 'if', got " + current().spelling);
    }
    auto if_stmt = current();
    auto t = next(); // consume 'if'

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

    auto if_true = parse_expr();

    if (t.type != tok::else_stmt) {
        throw std::runtime_error("Expected 'else', got " + current().spelling);
    }
    next(); // consume 'else'

    auto if_false = parse_expr();

    return make_expr<conditional_expr>(std::move(condition), std::move(if_true), std::move(if_false), if_stmt.loc);
}

// Floating point expression of the form:
// `float_pt` `unit`
expr parser::parse_float() {
    if (current().type != tok::floatpt) {
        throw std::runtime_error("Expected floating point number, got " + current().spelling);
    }
    auto num = current();
    auto t = next(); // consume float

    // todo: not enough!
    std::string unit;
    while (t.type!=tok::semicolon) {
        unit += t.spelling;
        t = next();
    }
    return make_expr<float_expr>(std::stold(num.spelling), unit, num.loc);
}

// Integer expression of the form:
// `integer` `unit`
expr parser::parse_int() {
    if (current().type != tok::integer) {
        throw std::runtime_error("Expected integer number, got " + current().spelling);
    }
    auto num = current();
    auto t = next(); // consume int

    // todo: not enough!
    std::string unit;
    while (t.type!=tok::semicolon) {
        unit += t.spelling;
        t = next();
    }
    return make_expr<int_expr>(std::stoll(num.spelling), unit, num.loc);
}

// Identifier expression of the form:
// `iden`
expr parser::parse_identifier() {
    if (current().type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + current().spelling);
    }
    auto t = current();
    next(); // consume identifier;
    return make_expr<identifier_expr>(t.spelling, t.loc);
}

// Identifier expression of the form:
// `iden`[:`type`]
expr parser::parse_typed_identifier() {
    if (current().type != tok::identifier) {
        throw std::runtime_error("Expected identifier, got " + current().spelling);
    }
    auto iden = current();
    auto t = next(); // consume identifier;
    if (t.type == tok::colon) {
        next(); // consume colon;
        auto type = parse_type();
        return make_expr<identifier_expr>(type, iden.spelling, iden.loc);
    }
    return make_expr<identifier_expr>(iden.spelling, iden.loc);
}

// Prefixed operations: exp, exprelr, log, cos, sin, abs, !, -, +, max, min.
expr parser::parse_prefix_expr() {
    auto prefix_op = current();
    switch (prefix_op.type) {
        case tok::exp:
        case tok::exprelr:
        case tok::log:
        case tok::cos:
        case tok::sin:
        case tok::abs: {
            auto t = next(); // consume op
            if (t.type != tok::lparen) {
                throw std::runtime_error("Expected '(' expression, got " + current().spelling);
            }
            next(); // consume '('
            auto e = parse_expr();
            t = current();
            if (t.type != tok::rparen) {
                throw std::runtime_error("Expected ')' expression, got " + current().spelling);
            }
            next(); // consume ')'
            return make_expr<unary_expr>(prefix_op.type, std::move(e), prefix_op.loc);
        }
        case tok::lnot:
        case tok::minus:
            next(); // consume '-'
            return make_expr<unary_expr>(prefix_op.type, parse_expr(), prefix_op.loc);
        case tok::plus:
            next(); // consume '+'
            return parse_expr();
        case tok::max:
        case tok::min: {
            auto t = next(); // consume op
            if (t.type != tok::lparen) {
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
        default: throw std::runtime_error("Expected prefix operator, got " + prefix_op.spelling);
    }
}

// This is one of:
// - call_expr
// - object_expr
// - identifier_expr
// - let_expr
// - with_expr
// - conditional_expr
// - float_expr
// - int_expr
// - prefix_expr
expr parser::parse_value_expr() {
    auto t = current();
    switch (t.type) {
        // Every
        case tok::lparen: {
            next(); // consume '('
            auto e = parse_expr();
            t = current();
            if (t.type != tok::rparen) {
                throw std::runtime_error("Expected ')' expression, got " + t.spelling);
            }
            next(); // consume ')'
            return e;
        }
        case tok::rbrace: {
            return parse_object();
        }
        case tok::identifier:
            if (peek().type == tok::lparen) {
                return parse_call();
            }
            if (peek().type == tok::lbrace) {
                return parse_object();
            }
            return parse_identifier();
        case tok::let:
            return parse_let();
        case tok::with:
            return parse_with();
        case tok::if_stmt:
            return parse_conditional();
        case tok::floatpt:
            return parse_float();
        case tok::integer:
            return parse_int();
        default:
            return parse_prefix_expr();
    }
}

// Handles infix operations
expr parser::parse_binary_expr(expr&& lhs, const token& lop) {
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
        rhs = parse_binary_expr(std::move(rhs), rop);
        return make_expr<binary_expr>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
    }
    else {
        lhs = make_expr<binary_expr>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
        return parse_binary_expr(std::move(lhs), rop);
    }
}

// Handles full nested value expressions
expr parser::parse_expr(int prec) {
    auto lhs = parse_value_expr();

    // Combine all sub-expressions with precedence greater than prec.
    for (;;) {
        auto op = current();
        auto op_prec = op.precedence();

        // Note: all tokens that are not infix binary operators have a precedence of -1,
        // so expressions like function calls will short circuit this loop here.
        if (op_prec <= prec) return lhs;

        next(); // consume the infix binary operator

        lhs = parse_binary_expr(std::move(lhs), op);
    }
}

// Type expressions
// Handles infix type operations
t_expr parser::parse_binary_type(t_expr&& lhs, const token& lop) {
    auto lop_prec = lop.precedence();
    auto rhs = parse_quantity_type(lop_prec);

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

t_expr parser::parse_quantity_type(int prec) {
    t_expr type;
    auto t = current();
    if (t.quantity()) {
        type = make_t_expr<quantity_type>(t.type, t.loc);
    }
    else if (t.type == tok::integer) {
        type = make_t_expr<integer_type>(std::stoll(t.spelling), t.loc);
    }
    else {
        throw std::runtime_error("Expected quantity or integer token, got " + t.spelling);
    }
    next(); // consume 'quantity'

    // Combine all sub-expressions with precedence greater than prec.
    for (;;) {
        auto op = current();
        auto op_prec = op.precedence();

        // Note: all tokens that are not infix binary operators have a precedence of -1,
        // so expressions like function calls will short circuit this loop here.
        if (op_prec <= prec) return type;

        next(); // consume the infix binary operator

        type = parse_binary_type(std::move(type), op);
    }
}

t_expr parser::parse_record_type() {
    auto t = current();
    if (t.type != tok::lbrace) {
        throw std::runtime_error("Expected '{', got " + t.spelling );
    }
    auto loc = t.loc;
    t = next(); // consume {

    std::vector<std::pair<std::string,t_expr>> field_types;
    while (t.type != tok::rbrace) {
        if (t.type != tok::identifier) {
            throw std::runtime_error("Expected identifier, got " + current().spelling);
        }
        std::string field_name = t.spelling;
        t = next(); // consume identifier

        if (t.type != tok::colon) {
            throw std::runtime_error("Expected ':', got " + current().spelling);
        }
        next(); // consume :

        field_types.emplace_back(field_name, parse_type());

        t = current();
        if (t.type == tok::rbrace) break;
        if (t.type != tok::comma) {
            throw std::runtime_error("Expected ',', got " + current().spelling);
        }
        t = next(); // consume ','
    }
    if (t.type != tok::rbrace) {
        throw std::runtime_error("Expected '}', got " + current().spelling);
    }
    next(); // consume '}'
    return make_t_expr<record_type>(field_types, loc);
}

t_expr parser::parse_type() {
    t_expr lhs;
    auto t = current();
    if (t.type == tok::identifier) {
        next(); // consume identifier
        return make_t_expr<record_alias_type>(t.spelling, t.loc);
    }
    if (t.type == tok::lbrace) {
        return parse_record_type();
    }
    auto type = parse_quantity_type();
    if (std::get_if<integer_type>(type.get())) {
        throw std::runtime_error("Can not have an integer as a type.");
    }
    return type;
}
} // namespace al