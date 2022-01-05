#include <optional>
#include <stdexcept>
#include <string>
#include <iostream>

#define FMT_HEADER_ONLY YES
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/compile.h>

#include <arblang/parser.hpp>
#include <arblang/token.hpp>

namespace al {

parser::parser(const std::string& description): lexer(description.c_str()) {}

void parser::parse() {
    auto t = current();
    while (t.type != tok::eof) {
        switch (t.type) {
            case tok::mechanism:
                mechanisms_.push_back(parse_mechanism());
                break;
            case tok::error:
                throw std::runtime_error(fmt::format("error {} at {}", t.spelling, to_string(t.loc)));
            default:
                throw std::runtime_error(fmt::format("Unexpected token {} at {}", t.spelling, to_string(t.loc)));
        }
        t = current();
    }
}

expr parser::parse_mechanism() {
    mechanism_expr m;
    auto t = current();
    m.loc = t.loc;
    if (t.type != tok::mechanism) {
        throw std::runtime_error(fmt::format("Unexpected token '{}' at {}, expected identifier", t.spelling, to_string(t.loc)));
    }
    t = next(); // consume mechanism

    if (!m.set_kind(t.type)) {
        throw std::runtime_error(fmt::format("Unexpected token '{}' at {}, expected mechanism kind identifier", t.spelling, to_string(t.loc)));
    }
    t = next(); // consume kind

    if (t.type != tok::quoted) {
        throw std::runtime_error(fmt::format("Unexpected token '{}' at {}, expected string between quotes", t.spelling, to_string(t.loc)));
    }
    m.name = t.spelling;

    t = next(); // consume quoted
    if (t.type != tok::lbrace) {
        throw std::runtime_error(fmt::format("Unexpected token '{}' at {}, expected '{'", t.spelling, to_string(t.loc)));
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
            case tok::state:
                m.states.push_back(parse_state());
                break;
            case tok::bind:
                m.bindings.push_back(parse_binding());
                break;
            case tok::record:
                m.records.push_back(parse_record_alias());
                break;
            case tok::function:
                m.functions.push_back(parse_function());
                break;
            case tok::effect:
                m.effects.push_back(parse_effect());
                break;
            case tok::evolve:
                m.evolutions.push_back(parse_evolve());
                break;
            case tok::initial:
                m.initializations.push_back(parse_initial());
                break;
            case tok::param_export:
                m.exports.push_back(parse_export());
                break;
            default:
                throw std::runtime_error(fmt::format("Unexpected token '{}' at {}", t.spelling, to_string(t.loc)));
        }
        t = current();
    }
    if (t.type != tok::rbrace) {
        throw std::runtime_error(fmt::format("Unexpected token '{}' at {}, expected '}'", t.spelling, to_string(t.loc)));
    }
    next(); // consume '}'

    return make_expr<mechanism_expr>(std::move(m));
}

// A parameter declaration of the form:
// parameter `iden` [: `type`] = `value_expression`;
expr parser::parse_parameter() {
    auto t = current();
    if (t.type != tok::parameter) {
        throw std::runtime_error(fmt::format("Expected parameter, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto loc = t.loc;
    next(); // consume 'parameter'

    auto assign = parse_assignment();
    return make_expr<parameter_expr>(std::move(assign.first), std::move(assign.second), loc);
};

// A constant declaration of the form:
// constant `iden` [: `type`] = `value_expression`;
expr parser::parse_constant()  {
    auto t = current();
    if (t.type != tok::constant) {
        throw std::runtime_error(fmt::format("Expected constant, got {} at {} ", t.spelling, to_string(t.loc)));
    }
    auto loc = t.loc;
    next(); // consume 'constant'

    auto assign = parse_assignment();
    return make_expr<constant_expr>(std::move(assign.first), std::move(assign.second), loc);
};

// A state declaration of the form:
// state `iden` [: `type`];
expr parser::parse_state()  {
    auto t = current();
    if (t.type != tok::state) {
        throw std::runtime_error(fmt::format("Expected `state`, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto loc = t.loc;
    next(); // consume 'state'

    auto iden = parse_typed_identifier();

    t = current();
    if (t.type == tok::eq) {
        throw std::runtime_error(fmt::format("'state' variables can only be initialized using an 'initial' statement"));
    }
    if (t.type != tok::semicolon) {
        throw std::runtime_error(fmt::format("Expected ;, got {} at {}", current().spelling, to_string(t.loc)));
    }
    next(); // consume ';'
    return make_expr<state_expr>(std::move(iden), loc);
};

// A record definition (type alias) of the form:
// record `iden` {`field_id0`: `type0`, `field_id1`: `type_1`, ...}[;]
expr parser::parse_record_alias()    {
    auto t = current();
    if (t.type != tok::record) {
        throw std::runtime_error(fmt::format("Expected `record`, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto loc = t.loc;
    t = next(); // consume 'record'

    if (t.type != tok::identifier) {
        throw std::runtime_error(fmt::format("Expected identifier, got {} at {}", t.spelling, to_string(t.loc)));
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
        throw std::runtime_error(fmt::format("Expected `function`, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto loc = t.loc;
    t = next(); // consume 'function'

    if (t.type != tok::identifier) {
        throw std::runtime_error(fmt::format("Expected identifier, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto name = t.spelling;
    t = next(); // consume identifier

    if (t.type != tok::lparen) {
        throw std::runtime_error(fmt::format("Expected (, got {} at {}", t.spelling, to_string(t.loc)));
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
            throw std::runtime_error(fmt::format("Expected `,`, got {} at {}", t.spelling, to_string(t.loc)));
        }
        t = next(); // consume ','
    }

    if (t.type != tok::rparen) {
        throw std::runtime_error(fmt::format("Expected ), got {} at {}", t.spelling, to_string(t.loc)));
    }
    t = next(); // consume ')'

    std::optional<t_expr> ret_quantity = {};
    if (t.type == tok::colon) {
        next(); // consume :
        ret_quantity = parse_type();
    }

    t = current();
    if (t.type != tok::lbrace) {
        throw std::runtime_error(fmt::format("Expected {, got {} at {}", t.spelling, to_string(t.loc)));
    }
    t = next(); // consume '{'

    if (t.type == tok::rbrace) {
        throw std::runtime_error(fmt::format("Expected expression, got }"));
    }
    auto ret_value = parse_expr();
    t = current();

    if (t.type == tok::semicolon) {
        t = next(); // consume ;
    }

    if (t.type != tok::rbrace) {
        throw std::runtime_error(fmt::format("Expected }, got {} at {}", t.spelling, to_string(t.loc)));
    }
    t = next(); // consume '}'

    if (t.type == tok::semicolon) {
        next(); // consume ;
    }

    return make_expr<function_expr>(name, args, ret_quantity, ret_value, loc);
};

// A binding of the form
// bind `iden`[:`type`] = `bindable`;
expr parser::parse_binding() {
    auto t = current();
    if (t.type != tok::bind) {
        throw std::runtime_error(fmt::format("Expected `bind`, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto loc = t.loc;
    next(); // consume 'bind'

    auto iden = parse_typed_identifier();

    t = current();
    if (t.type != tok::eq) {
        throw std::runtime_error(fmt::format("Expected =, got {} at {}", t.spelling, to_string(t.loc)));
    }
    t = next(); // consume =

    if(!t.bindable()) {
        throw std::runtime_error(fmt::format("Expected a valid bindable, got {} at {} ", t.spelling, to_string(t.loc)));
    }

    auto bindable = t;
    t = next(); // consume bindable

    std::string ion_name;
    if(bindable.ion_bindable()) {
        if (t.type != tok::lparen) {
            throw std::runtime_error(fmt::format("Expected (, got {} at {}", t.spelling, to_string(t.loc)));
        }
        t = next(); // consume (

        if (t.type != tok::quoted) {
            throw std::runtime_error(fmt::format("Unexpected token '{}' at {}, expected string between quotes", t.spelling, to_string(t.loc)));
        }
        ion_name = t.spelling;

        t = next();  // consume quoted
        if (t.type != tok::rparen) {
            throw std::runtime_error(fmt::format("Expected ), got {} at {}", t.spelling, to_string(t.loc)));
        }
        t = next(); // consume )
    }

    if (t.type == tok::semicolon) {
        next(); // consume ';'
    }
    return make_expr<bind_expr>(std::move(iden), bindable, ion_name, loc);
}

// An initialization of the form
// `initial` `identifier`[:`type`] = `value_expression`;
expr parser::parse_initial() {
    auto t = current();
    if (t.type != tok::initial) {
        throw std::runtime_error(fmt::format("Expected `initial`, got {} at {}" + t.spelling, to_string(t.loc)));
    }
    auto loc = t.loc;
    next(); // consume 'initial'

    auto assign = parse_assignment();
    return make_expr<initial_expr>(std::move(assign.first), std::move(assign.second), loc);
}

// An effect of the form
// `effect` `affectable`[:`type`] = `value_expression`;
expr parser::parse_effect() {
    auto t = current();
    if (t.type != tok::effect) {
        throw std::runtime_error(fmt::format("Expected `effect`, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto loc = t.loc;
    t = next(); // consume 'effect'

    if(!t.affectable()) {
        throw std::runtime_error(fmt::format("Expected a valid effect, got {} at {}" + t.spelling, to_string(t.loc)));
    }
    auto affectable = t;

    t = next(); // consume affectable;

    // Get ion if it exists
    std::string ion_name;
    if (t.type == tok::lparen) {
        t = next(); // consume (
        if (t.type != tok::quoted) {
            throw std::runtime_error(fmt::format("Unexpected token '{}' at {}, expected string between quotes", t.spelling, to_string(t.loc)));
        }
        ion_name = t.spelling;
        t = next(); // consume quoted
        if (t.type != tok::rparen) {
            throw std::runtime_error(fmt::format("Expected ), got {} at {}", t.spelling, to_string(t.loc)));
        }
        t = next(); // consume )
    }

    // Get type if it exists
    std::optional<t_expr> type = {};
    if (t.type == tok::colon) {
        next(); // consume colon;
        type = parse_type();
    }

    t = current();
    if (t.type != tok::eq) {
        throw std::runtime_error(fmt::format("Expected =, got {} at {}", t.spelling, to_string(t.loc)));
    }
    next(); // consume '='

    auto value = parse_expr();

    t = current();
    if (t.type != tok::semicolon) {
        throw std::runtime_error(fmt::format("Expected ;, got {} at {}", t.spelling, to_string(t.loc)));
    }
    next(); // consume ';'

    return make_expr<effect_expr>(affectable, ion_name, type, value, loc);
}

// An evolution of the form
// `evolve` `identifier`[:`type`] = `value_expression`;
expr parser::parse_evolve() {
    auto t = current();
    if (t.type != tok::evolve) {
        throw std::runtime_error("Expected `evolve`, got " + t.spelling);
    }
    auto loc = t.loc;
    next(); // consume 'evolve'

    auto assign = parse_assignment();
    return make_expr<evolve_expr>(std::move(assign.first), std::move(assign.second), loc);
}

// An export of the form
// `export` `identifier`[;]
expr parser::parse_export() {
    auto t = current();
    if (t.type != tok::param_export) {
        throw std::runtime_error("Expected `export`, got " + t.spelling);
    }
    auto loc = t.loc;
    next(); // consume 'export'

    auto iden = parse_identifier();

    t = current();
    if (current().type == tok::semicolon) {
        next(); // consume ';'
    }
    return make_expr<export_expr>(std::move(iden), loc);
}

// A function call of the form:
// `func_name`(`value_expression0`, `value_exprssion1`, ...)
expr parser::parse_call() {
    auto t = current();
    if (t.type != tok::identifier) {
        throw std::runtime_error(fmt::format("Expected identifier, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto iden = t.spelling;
    auto loc = t.loc;
    t = next(); // consume function name

    if (t.type != tok::lparen) {
        throw std::runtime_error(fmt::format("Expected '(', got {} at {}", t.spelling, to_string(t.loc)));
    }
    t = next(); // consume '('

    std::vector<expr> args;
    while (t.type != tok::rparen) {
        args.emplace_back(parse_expr());

        t = current();
        if (t.type == tok::rparen) break;

        // Check for comma between arguments
        if (t.type != tok::comma) {
            throw std::runtime_error(fmt::format("Expected ',' between function arguments, got {} at {}", t.spelling, to_string(t.loc)));
        }
        t = next(); // consume ','
    }

    if (t.type != tok::rparen) {
        throw std::runtime_error(fmt::format("Expected ')', got {} at {}", t.spelling, to_string(t.loc)));
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
        throw std::runtime_error(fmt::format("Expected '{', got {} at {}", t.spelling, to_string(t.loc)));
    }
    t = next(); // consume '{'

    std::vector<expr> fields, values;
    while (t.type != tok::rbrace) {
        fields.emplace_back(parse_typed_identifier());
        t = current();
        if (t.type != tok::eq) {
            throw std::runtime_error(fmt::format("Expected '=', got {} at {}", t.spelling, to_string(t.loc)));
        }
        next(); // consume '='
        values.emplace_back(parse_expr());

        // Check for semicolon between field assignments
        t = current();
        if (t.type != tok::semicolon) {
            throw std::runtime_error(fmt::format("Expected ';' between record fields, got {}, {}", t.spelling, to_string(t.loc)));
        }
        t = next(); // consume ';'
    }

    if (t.type != tok::rbrace) {
        throw std::runtime_error(fmt::format("Expected '}', got {} at {}", t.spelling, to_string(t.loc)));
    }
    next(); // consume '}'

    return make_expr<object_expr>(record_name, std::move(fields), std::move(values), loc);
}

// A let expression of the form:
// let `iden`[: `type`] = `value_expression0`; `value_expression1`
expr parser::parse_let() {
    auto t = current();
    if (t.type != tok::let) {
        throw std::runtime_error(fmt::format("Expected 'let', got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto let = current();
    next(); // consume 'let'

    auto iden = parse_typed_identifier();

    t = current();
    if (t.type != tok::eq) {
        throw std::runtime_error(fmt::format("Expected '=', got {} at {}", t.spelling, to_string(t.loc)));
    }
    next(); // consume '='

    auto value = parse_expr();

    t = current();
    if (t.type != tok::semicolon) {
        throw std::runtime_error(fmt::format("Expected ';', got {} at {}", t.spelling, to_string(t.loc)));
    }
    next(); // consume ';'
    auto body = parse_expr();
    return make_expr<let_expr>(std::move(iden), std::move(value), std::move(body), let.loc);
}

// A with expression of the form:
// with `iden`; `value_expression`
expr parser::parse_with() {
    auto t = current();
    if (t.type != tok::with) {
        throw std::runtime_error(fmt::format("Expected 'with', got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto with = t;
    t = next(); // consume 'with'

    auto val = parse_expr();
    t = current(); // consume identifier

    if (t.type != tok::semicolon) {
        throw std::runtime_error(fmt::format("Expected ';', got {} at {}", t.spelling, to_string(t.loc)));
    }
    next(); // consume ';'

    auto body = parse_expr();

    return make_expr<with_expr>(std::move(val), std::move(body), with.loc);
}

// A conditional expression of the form:
// if `boolean_expression` then `value_expression0` else `value_expression1`
expr parser::parse_conditional() {
    auto t = current();
    if (t.type != tok::if_stmt) {
        throw std::runtime_error(fmt::format("Expected 'if', got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto if_stmt = current();
    t = next(); // consume 'if'

    auto condition = parse_expr();

    t = current();
    if (t.type != tok::then_stmt) {
        throw std::runtime_error("Expected 'then', got " + current().spelling);
    }
    next(); // consume 'else'

    auto if_true = parse_expr();

    t = current();
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
    auto t = current();
    if (t.type != tok::floatpt) {
        throw std::runtime_error(fmt::format("Expected floating point number, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto num = current();
    t = next(); // consume float

    return make_expr<float_expr>(std::stold(num.spelling), try_parse_unit(), num.loc);
}

// Integer expression of the form:
// `integer` `unit`
expr parser::parse_int() {
    auto t = current();
    if (t.type != tok::integer) {
        throw std::runtime_error(fmt::format("Expected integer number, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto num = current();
    t = next(); // consume int

    return make_expr<int_expr>(std::stoll(num.spelling), try_parse_unit(), num.loc);
}

// Identifier expression of the form:
// `iden`
expr parser::parse_identifier() {
    auto t = current();
    if (current().type != tok::identifier) {
        throw std::runtime_error(fmt::format("Expected identifier, got {} at {}", t.spelling, to_string(t.loc)));
    }
    t = current();
    next(); // consume identifier;
    return make_expr<identifier_expr>(t.spelling, t.loc);
}

// Identifier expression of the form:
// `iden`[:`type`]
expr parser::parse_typed_identifier() {
    auto t = current();
    if (t.type != tok::identifier) {
        throw std::runtime_error(fmt::format("Expected identifier, got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto iden = current();
    t = next(); // consume identifier;
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
                throw std::runtime_error(fmt::format("Expected '(' expression, got {} at {}", t.spelling, to_string(t.loc)));
            }
            next(); // consume '('
            auto e = parse_expr();
            t = current();
            if (t.type != tok::rparen) {
                throw std::runtime_error(fmt::format("Expected ')' expression, got {} at {}", t.spelling, to_string(t.loc)));
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
                throw std::runtime_error(fmt::format("Expected '(' expression, got {} at {}", t.spelling, to_string(t.loc)));
            }
            next(); // consume '('
            auto lhs = parse_expr();
            t = current();
            if (t.type != tok::comma) {
                throw std::runtime_error(fmt::format("Expected ',' expression, got {} at {}", t.spelling, to_string(t.loc)));
            }
            next(); // consume ','
            auto rhs = parse_expr();
            t = current();
            if (t.type != tok::rparen) {
                throw std::runtime_error(fmt::format("Expected ')' expression, got {} at {}", t.spelling, to_string(t.loc)));
            }
            next(); // consume ')'
            return make_expr<binary_expr>(prefix_op.type, std::move(lhs), std::move(rhs), prefix_op.loc);
        }
        default: throw std::runtime_error(fmt::format("Expected prefix operator, got {} at {}", prefix_op.spelling, to_string(prefix_op.loc)));
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
                throw std::runtime_error(fmt::format("Expected ')' expression, got ", t.spelling, to_string(t.loc)));
            }
            next(); // consume ')'
            return e;
        }
        case tok::lbrace: {
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
        throw std::runtime_error("parse_binary_expr() : encountered operator of higher precedence");
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
        throw std::runtime_error("parse_binary_type() : encountered operator of higher precedence");
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

// Handles parenthesis and integer signs
t_expr parser::parse_type_element() {
    auto t = current();
    if (t.quantity()) {
        next(); // consume identifier
        return make_t_expr<quantity_type>(t.type, t.loc);
    }
    switch (t.type) {
        case tok::lparen: {
            next(); // consume '('
            auto type = parse_quantity_type();
            t = current();
            if (t.type != tok::rparen) {
                throw std::runtime_error(fmt::format("Expected ')' expression, got {} at {}", t.spelling, to_string(t.loc)));
            }
            next(); // consume ')'
            return type;
        }
        case tok::minus: {
            auto t = next();
            if (t.type == tok::integer) {
                next(); // consume integer
                return make_t_expr<integer_type>(-1*std::stoll(t.spelling), t.loc);
            }
            throw std::runtime_error(fmt::format("Expected integer after '-' token in type expression, got {} at {}", t.spelling, to_string(t.loc)));
        }
        case tok::plus: {
            auto t = next();
            if (t.type == tok::integer) {
                next(); // consume integer
                return make_t_expr<integer_type>(std::stoll(t.spelling), t.loc);
            }
            throw std::runtime_error(fmt::format("Expected integer after '+' token in type expression, got {} at {}", t.spelling, to_string(t.loc)));
        }
        case tok::integer: {
            next(); // consume integer
            return make_t_expr<integer_type>(std::stoll(t.spelling), t.loc);
        }
        default: throw std::runtime_error(fmt::format("Uexpected token in type expression {} at {}", t.spelling, to_string(t.loc)));
    }
}

t_expr parser::parse_quantity_type(int prec) {
    auto type = parse_type_element();

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
        throw std::runtime_error(fmt::format("Expected '{', got {} at {}", t.spelling, to_string(t.loc)));
    }
    auto loc = t.loc;
    t = next(); // consume {

    std::vector<std::pair<std::string,t_expr>> field_types;
    while (t.type != tok::rbrace) {
        if (t.type != tok::identifier) {
            throw std::runtime_error(fmt::format("Expected identifier, got {} at {}", t.spelling, to_string(t.loc)));
        }
        std::string field_name = t.spelling;
        t = next(); // consume identifier

        if (t.type != tok::colon) {
            throw std::runtime_error(fmt::format("Expected ':', got {}", t.spelling, to_string(t.loc)));
        }
        next(); // consume :

        field_types.emplace_back(field_name, parse_type());

        t = current();
        if (t.type == tok::rbrace) break;
        if (t.type != tok::comma) {
            throw std::runtime_error(fmt::format("Expected ',', got {} at {}", t.spelling, to_string(t.loc)));
        }
        t = next(); // consume ','
    }
    if (t.type != tok::rbrace) {
        throw std::runtime_error(fmt::format("Expected '}', got {} at {}", t.spelling, to_string(t.loc)));
    }
    t = next(); // consume '}'
    if (t.type == tok::semicolon) {
        next(); // consume ';'
    }
    return make_t_expr<record_type>(field_types, loc);
}

t_expr parser::parse_type() {
    auto t = current();
    if (t.type == tok::identifier) {
        next(); // consume identifier
        return make_t_expr<record_alias_type>(t.spelling, t.loc);
    }
    if (t.type == tok::lbrace) {
        return parse_record_type();
    }
    auto type = parse_quantity_type();
    if (!verify_type(type)) {
        throw std::runtime_error("Invalid type.");
    }
    return std::move(type);
}

// Unit expressions
// Handles infix unit operations
u_expr parser::parse_binary_unit(u_expr&& lhs, const token& lop) {
    auto lop_prec = lop.precedence();
    auto rhs = parse_unit_expr(lop_prec);

    auto rop = current();
    auto rop_prec = rop.precedence();

    if (rop_prec > lop_prec) {
        throw std::runtime_error("parse_binary_unit() : encountered operator of higher precedence");
    }
    if (rop_prec < lop_prec) {
        return make_u_expr<binary_unit>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
    }

    next(); // consume rop
    if (rop.right_associative()) {
        rhs = parse_binary_unit(std::move(rhs), rop);
        return make_u_expr<binary_unit>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
    }
    else {
        auto bin_unit = make_u_expr<binary_unit>(lop.type, std::move(lhs), std::move(rhs), lop.loc);
        return parse_binary_unit(std::move(bin_unit), rop);
    }
}

u_expr parser::parse_unit_element() {
    auto t = current();
    switch (t.type) {
        case tok::lparen: {
            next(); // consume '('
            auto unit = parse_unit_expr();
            t = current();
            if (t.type != tok::rparen) {
                throw std::runtime_error(fmt::format("Expected ')' expression, got {} at {}", t.spelling, to_string(t.loc)));
            }
            next(); // consume ')'
            return unit;
        }
        case tok::minus: {
            auto t = next();
            if (t.type == tok::integer) {
                next(); // consume integer
                return make_u_expr<integer_unit>(-1*std::stoll(t.spelling), t.loc);
            }
            throw std::runtime_error(fmt::format("Expected integer after '-' token in type expression, got {} at {}", t.spelling, to_string(t.loc)));
        }
        case tok::plus: {
            auto t = next();
            if (t.type == tok::integer) {
                next(); // consume integer
                return make_u_expr<integer_unit>(std::stoll(t.spelling), t.loc);
            }
            throw std::runtime_error(fmt::format("Expected integer after '+' token in type expression, got {} at {}", t.spelling, to_string(t.loc)));
        }
        case tok::integer: {
            next(); // consume integer
            return make_u_expr<integer_unit>(std::stoll(t.spelling), t.loc);
        }
        case tok::identifier: {
            if (auto u = check_simple_unit(t.spelling)) {
                next(); // consume identifier
                return make_u_expr<simple_unit>(u.value(), t.loc);
            }
        }
        default: throw std::runtime_error("Uexpected token in unit expression: " + t.spelling);
    }
}

u_expr parser::parse_unit_expr(int prec) {
    auto unit = parse_unit_element();

    // Combine all sub-expressions with precedence greater than prec.
    for (;;) {
        auto op = current();
        auto op_prec = op.precedence();

        // All tokens that are not infix binary operators have a precedence of -1,
        // so expressions like function calls will short circuit this loop here.
        if (op_prec <= prec) return unit;

        next(); // consume the infix binary operator

        unit = parse_binary_unit(std::move(unit), op);
    }
}

u_expr parser::try_parse_unit(int prec) {
    auto t = current();
    if (t.type != tok::lbracket) {
        return make_u_expr<no_unit>();
    }
    next(); // consume [
    auto u = parse_unit_expr(prec);
    t = current();
    if (t.type != tok::rbracket) {
        throw std::runtime_error(fmt::format("Expected ']', got {} at {}", t.spelling, to_string(t.loc)));
    }
    next();
    if (!verify_unit(u)) {
        throw std::runtime_error("Invalid unit.");
    }
    return std::move(u);
}

std::pair<expr, expr> parser::parse_assignment()  {
    auto iden = parse_typed_identifier();
    auto t = current();
    if (t.type != tok::eq) {
        throw std::runtime_error(fmt::format("Expected =, got {} at {}", t.spelling, to_string(t.loc)));
    }
    next(); // consume '='

    auto value = parse_expr();

    t = current();
    if (t.type != tok::semicolon) {
        throw std::runtime_error(fmt::format("Expected ;, got {} at {}", t.spelling, to_string(t.loc)));
    }
    next(); // consume ';'

    return {iden, value};
};
} // namespace al