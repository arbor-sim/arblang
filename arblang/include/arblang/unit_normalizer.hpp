#pragma once

#include <optional>
#include <string>
#include <vector>

#include <arblang/raw_expressions.hpp>

namespace al {
using namespace raw_ir;

class unit_normalizer {
public:
    unit_normalizer(expr e);
    expr normalize();
private:
    static mechanism_expr normalize(const mechanism_expr& e);
    static expr normalize(const parameter_expr& e);
    static expr normalize(const constant_expr& e);
    static expr normalize(const state_expr& e);
    static expr normalize(const record_alias_expr& e);
    static expr normalize(const function_expr& e);
    static expr normalize(const bind_expr& e);
    static expr normalize(const initial_expr& e);
    static expr normalize(const evolve_expr& e);
    static expr normalize(const effect_expr& e);
    static expr normalize(const export_expr& expr);
    static expr normalize(const call_expr& expr);
    static expr normalize(const object_expr& expr);
    static expr normalize(const let_expr& expr);
    static expr normalize(const with_expr& expr);
    static expr normalize(const conditional_expr& expr);
    static expr normalize(const identifier_expr& expr);
    static expr normalize(const float_expr& expr);
    static expr normalize(const int_expr& expr);
    static expr normalize(const unary_expr& expr);
    static expr normalize(const binary_expr& expr);

    expr expression;
};

} // namespace al