#pragma once

#include <optional>
#include <string>
#include <vector>

#include <arblang/lexer.hpp>
#include <arblang/raw_expressions.hpp>
#include <arblang/type_expressions.hpp>

namespace al {
using namespace raw_ir;

class parser: lexer {
public:
    parser(std::string const&);
    void parse();

    const std::vector<expr>& modules() {return modules_;};

private:
    expr parse_module();
    expr parse_parameter();
    expr parse_constant();
    expr parse_record();
    expr parse_function();
    expr parse_import();

    expr parse_call();
    expr parse_field();
    expr parse_let();
    expr parse_conditional();
    expr parse_identifier();
    expr parse_float();
    expr parse_int();
    expr parse_prefix();
    expr parse_infix();

    expr parse_expr();

    type_expr parse_type();

    std::vector<expr> modules_;
};
} // namespace al