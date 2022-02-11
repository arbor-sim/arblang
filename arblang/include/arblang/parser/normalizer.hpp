#pragma once

#include <optional>
#include <string>
#include <vector>

#include <arblang/parser/parsed_expressions.hpp>

namespace al {
namespace parsed_ir {

parsed_mechanism normalize(const parsed_mechanism& e);
p_expr normalize(p_expr e);
p_expr normalize(const parsed_parameter& e);
p_expr normalize(const parsed_constant& e);
p_expr normalize(const parsed_state& e);
p_expr normalize(const parsed_record_alias& e);
p_expr normalize(const parsed_function& e);
p_expr normalize(const parsed_bind& e);
p_expr normalize(const parsed_initial& e);
p_expr normalize(const parsed_evolve& e);
p_expr normalize(const parsed_effect& e);
p_expr normalize(const parsed_export& p_expr);
p_expr normalize(const parsed_call& p_expr);
p_expr normalize(const parsed_object& p_expr);
p_expr normalize(const parsed_let& p_expr);
p_expr normalize(const parsed_with& p_expr);
p_expr normalize(const parsed_conditional& p_expr);
p_expr normalize(const parsed_identifier& p_expr);
p_expr normalize(const parsed_float& p_expr);
p_expr normalize(const parsed_int& p_expr);
p_expr normalize(const parsed_unary& p_expr);
p_expr normalize(const parsed_binary& p_expr);

} // namespace parsed_ir
} // namespace al