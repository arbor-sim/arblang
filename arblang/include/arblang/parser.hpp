#pragma once

#include <optional>
#include <string>
#include <vector>

#include <arblang/lexer.hpp>
#include <arblang/raw_expressions.hpp>
#include <arblang/type_expressions.hpp>
#include <arblang/unit_expressions.hpp>

namespace al {
using namespace raw_ir;
using namespace t_raw_ir;
using namespace u_raw_ir;

class parser: lexer {
public:
    parser(std::string const&);
    void parse();

    const std::vector<expr>& mechanisms() {return mechanisms_;};

    expr parse_mechanism();
    expr parse_parameter();
    expr parse_constant();
    expr parse_state();
    expr parse_record_alias();
    expr parse_function();
    expr parse_binding();
    expr parse_effect();
    expr parse_evolve();
    expr parse_initial();
    expr parse_export();

    expr parse_identifier();
    expr parse_typed_identifier();
    expr parse_call();
    expr parse_object();
    expr parse_let();
    expr parse_with();
    expr parse_conditional();
    expr parse_float();
    expr parse_int();

    expr parse_prefix_expr();
    expr parse_value_expr();
    expr parse_binary_expr(expr&&, const token&);
    expr parse_expr(int prec=0);

    t_expr parse_binary_type(t_expr&& lhs, const token& lop);
    t_expr parse_type_element();
    t_expr parse_quantity_type(int prec=0);
    t_expr parse_record_type();
    t_expr parse_type();

    u_expr parse_binary_unit(u_expr&& lhs, const token& lop);
    u_expr parse_unit_element();
    u_expr parse_unit_expr(int prec=0);
    std::optional<u_expr> try_parse_unit(int prec=0);

private:
    std::string parse_quoted();
    std::pair<expr, expr> parse_assignment();

    std::vector<expr> mechanisms_;
};
} // namespace al