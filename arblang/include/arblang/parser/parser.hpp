#pragma once

#include <optional>
#include <string>
#include <vector>

#include <arblang/parser/lexer.hpp>
#include <arblang/parser/parsed_expressions.hpp>
#include <arblang/parser/parsed_types.hpp>
#include <arblang/parser/parsed_units.hpp>

namespace al {
using namespace parsed_ir;
using namespace parsed_type_ir;
using namespace parsed_unit_ir;

class parser: lexer {
public:
    parser(std::string const&);
    void parse();

    const std::vector<parsed_mechanism>& mechanisms() {return mechanisms_;};
    parsed_mechanism parse_mechanism();

    p_expr parse_parameter();
    p_expr parse_constant();
    p_expr parse_state();
    p_expr parse_record_alias();
    p_expr parse_function();
    p_expr parse_binding();
    p_expr parse_effect();
    p_expr parse_evolve();
    p_expr parse_initial();
    p_expr parse_on_event();
    p_expr parse_export();

    p_expr parse_identifier();
    p_expr parse_typed_identifier();
    p_expr parse_call();
    p_expr parse_object();
    p_expr parse_let();
    p_expr parse_with();
    p_expr parse_conditional();
    p_expr parse_float();
    p_expr parse_int();

    p_expr parse_prefix_expr();
    p_expr parse_value_expr();
    p_expr parse_parsed_binary(p_expr&&, const token&);
    p_expr parse_expr(int prec=0);

    p_type parse_binary_type(p_type&& lhs, const token& lop);
    p_type parse_type_element();
    p_type parse_parsed_quantity_type(int prec=0);
    p_type parse_parsed_record_type();
    p_type parse_type();

    p_unit parse_parsed_binary_unit(p_unit&& lhs, const token& lop);
    p_unit parse_unit_element();
    p_unit parse_unit_expr(int prec=0);
    p_unit try_parse_unit(int prec=0);

private:
    std::pair<p_expr, p_expr> parse_assignment();

    std::vector<parsed_mechanism> mechanisms_;
};
} // namespace al