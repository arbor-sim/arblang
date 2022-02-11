#pragma once

#include <optional>
#include <string>
#include <vector>

#include <arblang/parser/raw_expressions.hpp>

namespace al {
namespace raw_ir {

mechanism_expr normalize(const mechanism_expr& e);
expr normalize(expr e);
expr normalize(const parameter_expr& e);
expr normalize(const constant_expr& e);
expr normalize(const state_expr& e);
expr normalize(const record_alias_expr& e);
expr normalize(const function_expr& e);
expr normalize(const bind_expr& e);
expr normalize(const initial_expr& e);
expr normalize(const evolve_expr& e);
expr normalize(const effect_expr& e);
expr normalize(const export_expr& expr);
expr normalize(const call_expr& expr);
expr normalize(const object_expr& expr);
expr normalize(const let_expr& expr);
expr normalize(const with_expr& expr);
expr normalize(const conditional_expr& expr);
expr normalize(const identifier_expr& expr);
expr normalize(const float_expr& expr);
expr normalize(const int_expr& expr);
expr normalize(const unary_expr& expr);
expr normalize(const binary_expr& expr);

} // namespace raw_ir
} // namespace al