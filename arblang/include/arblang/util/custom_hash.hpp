#pragma once

#include <cstddef>
#include <typeindex>

#include "arblang/resolver/resolved_expressions.hpp"

template <class T>
inline void hash_combine(std::size_t& s, const T& v)
{
    std::hash<T> h;
    s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
}

using namespace al::resolved_ir;

namespace std {
template <>
struct hash<resolved_quantity> {
    inline size_t operator()(const resolved_quantity& t) const {
        std::size_t res = 0;
        for (const auto& elem: t.type.quantity_exponents) {
            hash_combine(res, elem);
        }
        return res;
    }
};

template <>
struct hash<resolved_boolean> {
    inline size_t operator()(const resolved_boolean& t) const {
        std::hash<std::string> h;
        return h("");
    }
};

template <>
struct hash<resolved_record> {
    inline size_t operator()(const resolved_record& t) const {
        std::size_t res = 0;
        for (const auto& [id, val]: t.fields) {
            hash_combine(res, id);
            hash_combine(res, *val);
        }
        return res;
    }
};

template <>
struct hash<resolved_argument> {
    inline size_t operator()(const resolved_argument& e) const {
        std::size_t res = 0;
        hash_combine(res, e.name);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_variable> {
    inline size_t operator()(const resolved_variable& e) const {
        std::size_t res = 0;
        hash_combine(res, e.name);
        hash_combine(res, *e.value);
        hash_combine(res, *e.type);
        return res;
    }
};

template<>
struct hash<resolved_parameter> {
    inline size_t operator()(const resolved_parameter& e) const {
        std::size_t res = 0;
        hash_combine(res, e.name);
        hash_combine(res, *e.value);
        hash_combine(res, *e.type);
        return res;
    }
};

template<>
struct hash<resolved_constant> {
    inline size_t operator()(const resolved_constant& e) const {
        std::size_t res = 0;
        hash_combine(res, e.name);
        hash_combine(res, *e.value);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_state> {
    inline size_t operator()(const resolved_state& e) const {
        std::size_t res = 0;
        hash_combine(res, e.name);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_record_alias> {
    inline size_t operator()(const resolved_record_alias& e) const {
        std::size_t res = 0;
        hash_combine(res, e.name);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_function> {
    inline size_t operator()(const resolved_function& e) const {
        std::size_t res = 0;
        hash_combine(res, e.name);
        hash_combine(res, *e.type);
        for (const auto& a: e.args) {
            hash_combine(res, *a);
        }
        hash_combine(res, *e.body);
        return res;
    }
};

template <>
struct hash<resolved_bind> {
    inline size_t operator()(const resolved_bind& e) const {
        std::size_t res = 0;
        hash_combine(res, e.name);
        hash_combine(res, e.ion);
        hash_combine(res, e.bind);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_initial> {
    inline size_t operator()(const resolved_initial& e) const {
        std::size_t res = 0;
        hash_combine(res, *e.identifier);
        hash_combine(res, *e.value);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_on_event> {
    inline size_t operator()(const resolved_on_event& e) const {
        std::size_t res = 0;
        hash_combine(res, *e.argument);
        hash_combine(res, *e.identifier);
        hash_combine(res, *e.value);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_evolve> {
    inline size_t operator()(const resolved_evolve& e) const {
        std::size_t res = 0;
        hash_combine(res, *e.identifier);
        hash_combine(res, *e.value);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_effect> {
    inline size_t operator()(const resolved_effect& e) const {
        std::size_t res = 0;
        hash_combine(res, e.effect);
        hash_combine(res, *e.value);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_export> {
    inline size_t operator()(const resolved_export& e) const {
        std::size_t res = 0;
        hash_combine(res, e.identifier);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_call> {
    inline size_t operator()(const resolved_call& e) const {
        std::size_t res = 0;
        hash_combine(res, e.f_identifier);
        hash_combine(res, *e.type);
        for (const auto& a: e.call_args) {
            hash_combine(res, *a);
        }
        return res;
    }
};

template <>
struct hash<resolved_object> {
    inline size_t operator()(const resolved_object& e) const {
        std::size_t res = 0;
        hash_combine(res, *e.type);
        for (const auto& a: e.record_fields) {
            hash_combine(res, *a);
        }
        return res;
    }
};

template <>
struct hash<resolved_let> {
    inline size_t operator()(const resolved_let& e) const {
        std::size_t res = 0;
        hash_combine(res, *e.identifier);
        hash_combine(res, *e.body);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_conditional> {
    inline size_t operator()(const resolved_conditional& e) const {
        std::size_t res = 0;
        hash_combine(res, *e.condition);
        hash_combine(res, *e.value_true);
        hash_combine(res, *e.value_false);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_float> {
    inline size_t operator()(const resolved_float& e) const {
        std::size_t res = 0;
        hash_combine(res, e.value);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_int> {
    inline size_t operator()(const resolved_int& e) const {
        std::size_t res = 0;
        hash_combine(res, e.value);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_unary> {
    inline size_t operator()(const resolved_unary& e) const {
        std::size_t res = 0;
        hash_combine(res, e.op);
        hash_combine(res, *e.arg);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_binary> {
    inline size_t operator()(const resolved_binary& e) const {
        std::size_t res = 0;
        hash_combine(res, e.op);
        hash_combine(res, *e.lhs);
        hash_combine(res, *e.rhs);
        hash_combine(res, *e.type);
        return res;
    }
};

template <>
struct hash<resolved_field_access> {
    inline size_t operator()(const resolved_field_access& e) const {
        std::size_t res = 0;
        hash_combine(res, e.field);
        hash_combine(res, *e.object);
        hash_combine(res, *e.type);
        return res;
    }
};
}
