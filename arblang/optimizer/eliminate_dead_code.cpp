#include <cmath>
#include <string>
#include <unordered_set>

#include <arblang/optimizer/eliminate_dead_code.hpp>
#include <arblang/util/custom_hash.hpp>

namespace al {
namespace resolved_ir {

std::pair<resolved_mechanism, bool> eliminate_dead_code(const resolved_mechanism& e) {
    std::unordered_set<std::string> dead_code;
    std::unordered_set<std::string> dead_param;
    resolved_mechanism mech;

    bool made_changes = false;
    for (const auto& c: e.constants) {
        dead_param.insert(std::get<resolved_constant>(*c).name);
    }
    for (const auto& c: e.parameters) {
        dead_param.insert(std::get<resolved_parameter>(*c).name);
    }
    for (const auto& c: e.bindings) {
        dead_param.insert(std::get<resolved_bind>(*c).name);
    }
    for (const auto& c: e.states) {
        dead_param.insert(std::get<resolved_state>(*c).name);
    }
    for (const auto& c: e.functions) {
        find_dead_code(c, dead_param);
    }
    for (const auto& c: e.initializations) {
        find_dead_code(c, dead_param);
    }
    for (const auto& c: e.evolutions) {
        find_dead_code(c, dead_param);
    }
    for (const auto& c: e.effects) {
        find_dead_code(c, dead_param);
    }

    // Whatever remains in dead_const, dead_param, dead_bind, dead_state
    // wasn't actually used and can be skipped in the final mechanism

    for (const auto& c: e.constants) {
        auto name = std::get<resolved_constant>(*c).name;
        if (dead_param.count(name)) continue;

        find_dead_code(c, dead_code);
        if (!dead_code.empty()) {
            mech.constants.push_back(remove_dead_code(c, dead_code));
        } else {
            mech.constants.push_back(c);
        }
        made_changes |= !dead_code.empty();
    }
    for (const auto& c: e.parameters) {
        auto name = std::get<resolved_parameter>(*c).name;
        if (dead_param.count(name)) continue;

        dead_code.clear();
        find_dead_code(c, dead_code);
        if (!dead_code.empty()) {
            mech.parameters.push_back(remove_dead_code(c, dead_code));
        } else {
            mech.parameters.push_back(c);
        }
        made_changes |= !dead_code.empty();
    }
    for (const auto& c: e.bindings) {
        auto name = std::get<resolved_bind>(*c).name;
        if (dead_param.count(name)) continue;

        dead_code.clear();
        find_dead_code(c, dead_code);
        if (!dead_code.empty()) {
            mech.bindings.push_back(remove_dead_code(c, dead_code));
        } else {
            mech.bindings.push_back(c);
        }
        made_changes |= !dead_code.empty();
    }
    for (const auto& c: e.states) {
        auto name = std::get<resolved_state>(*c).name;
        if (dead_param.count(name)) continue;

        dead_code.clear();
        find_dead_code(c, dead_code);
        if (!dead_code.empty()) {
            mech.states.push_back(remove_dead_code(c, dead_code));
        } else {
            mech.states.push_back(c);
        }
        made_changes |= !dead_code.empty();
    }
    for (const auto& c: e.functions) {
        dead_code.clear();
        find_dead_code(c, dead_code);
        if (!dead_code.empty()) {
            mech.functions.push_back(remove_dead_code(c, dead_code));
        } else {
            mech.functions.push_back(c);
        }
        made_changes |= !dead_code.empty();
    }
    for (const auto& c: e.initializations) {
        dead_code.clear();
        find_dead_code(c, dead_code);
        if (!dead_code.empty()) {
            mech.initializations.push_back(remove_dead_code(c, dead_code));
        } else {
            mech.initializations.push_back(c);
        }
        made_changes |= !dead_code.empty();
    }
    for (const auto& c: e.evolutions) {
        dead_code.clear();
        find_dead_code(c, dead_code);
        if (!dead_code.empty()) {
            mech.evolutions.push_back(remove_dead_code(c, dead_code));
        } else {
            mech.evolutions.push_back(c);
        }
        made_changes |= !dead_code.empty();
    }
    for (const auto& c: e.effects) {
        dead_code.clear();
        find_dead_code(c, dead_code);
        if (!dead_code.empty()) {
            mech.effects.push_back(remove_dead_code(c, dead_code));
        } else {
            mech.effects.push_back(c);
        }
        made_changes |= !dead_code.empty();
    }
    for (const auto& c: e.exports) {
        dead_code.clear();
        find_dead_code(c, dead_code);
        if (!dead_code.empty()) {
            mech.exports.push_back(remove_dead_code(c, dead_code));
        } else {
            mech.exports.push_back(c);
        }
        made_changes |= !dead_code.empty();
    }
    mech.name = e.name;
    mech.loc = e.loc;
    mech.kind = e.kind;
    return {mech, made_changes};
}

// Find dead code
void find_dead_code(const resolved_record_alias& e, std::unordered_set<std::string>& dead_args) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation.");
}

void find_dead_code(const resolved_argument& e, std::unordered_set<std::string>& dead_args) {
    if (dead_args.count(e.name)) dead_args.erase(e.name);
}

void find_dead_code(const resolved_variable& e, std::unordered_set<std::string>& dead_args) {
    if (dead_args.count(e.name)) dead_args.erase(e.name);
}

void find_dead_code(const resolved_parameter& e, std::unordered_set<std::string>& dead_args) {
    find_dead_code(e.value, dead_args);
}

void find_dead_code(const resolved_constant& e, std::unordered_set<std::string>& dead_args) {
    find_dead_code(e.value, dead_args);
}

void find_dead_code(const resolved_state& e, std::unordered_set<std::string>& dead_args) {}

void find_dead_code(const resolved_function& e, std::unordered_set<std::string>& dead_args) {
    find_dead_code(e.body, dead_args);
}

void find_dead_code(const resolved_bind& e, std::unordered_set<std::string>& dead_args) {}

void find_dead_code(const resolved_initial& e, std::unordered_set<std::string>& dead_args) {
    find_dead_code(e.value, dead_args);
}

void find_dead_code(const resolved_evolve& e, std::unordered_set<std::string>& dead_args) {
    find_dead_code(e.value, dead_args);
}

void find_dead_code(const resolved_effect& e, std::unordered_set<std::string>& dead_args) {
    find_dead_code(e.value, dead_args);
}

void find_dead_code(const resolved_export& e, std::unordered_set<std::string>& dead_args) {}

void find_dead_code(const resolved_call& e, std::unordered_set<std::string>& dead_args) {
    for (const auto& a: e.call_args) {
        find_dead_code(a, dead_args);
    }
}

void find_dead_code(const resolved_object& e, std::unordered_set<std::string>& dead_args) {
    for (const auto& a: e.field_values()) {
        find_dead_code(a, dead_args);
    }
}

void find_dead_code(const resolved_let& e, std::unordered_set<std::string>& dead_args) {
    dead_args.insert(std::get<resolved_variable>(*e.identifier).name);
    find_dead_code(e.id_value(), dead_args);
    find_dead_code(e.body, dead_args);
}

void find_dead_code(const resolved_conditional& e,std::unordered_set<std::string>& dead_args) {
    find_dead_code(e.condition, dead_args);
    find_dead_code(e.value_true, dead_args);
    find_dead_code(e.value_false, dead_args);
}

void find_dead_code(const resolved_float& e, std::unordered_set<std::string>& dead_args) {}

void find_dead_code(const resolved_int& e, std::unordered_set<std::string>& dead_args) {}

void find_dead_code(const resolved_unary& e, std::unordered_set<std::string>& dead_args) {
    find_dead_code(e.arg, dead_args);
}

void find_dead_code(const resolved_binary& e, std::unordered_set<std::string>& dead_args) {
    find_dead_code(e.lhs, dead_args);
    find_dead_code(e.rhs, dead_args);
}

void find_dead_code(const resolved_field_access& e, std::unordered_set<std::string>& dead_args) {
    find_dead_code(e.object, dead_args);
}

void find_dead_code(const r_expr& e, std::unordered_set<std::string>& dead_args) {
    return std::visit([&](auto& c) {return find_dead_code(c, dead_args);}, *e);
}

// Remove dead code
r_expr remove_dead_code(const resolved_record_alias& e, const std::unordered_set<std::string>& dead_args) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation.");
}

r_expr remove_dead_code(const resolved_parameter& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_parameter>(e.name, remove_dead_code(e.value, dead_args), e.type, e.loc);
}

r_expr remove_dead_code(const resolved_argument& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_argument>(e);
}

r_expr remove_dead_code(const resolved_variable& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_variable>(e);
}

r_expr remove_dead_code(const resolved_constant& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_constant>(e.name, remove_dead_code(e.value, dead_args), e.type, e.loc);
}

r_expr remove_dead_code(const resolved_state& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_state>(e);
}

r_expr remove_dead_code(const resolved_function& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_function>(e.name, e.args, remove_dead_code(e.body, dead_args), e.type, e.loc);
}

r_expr remove_dead_code(const resolved_bind& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_bind>(e);
}

r_expr remove_dead_code(const resolved_initial& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_initial>(e.identifier, remove_dead_code(e.value, dead_args), e.type, e.loc);
}

r_expr remove_dead_code(const resolved_evolve& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_evolve>(e.identifier, remove_dead_code(e.value, dead_args), e.type, e.loc);
}

r_expr remove_dead_code(const resolved_effect& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_effect>(e.effect, e.ion, remove_dead_code(e.value, dead_args), e.type, e.loc);
}

r_expr remove_dead_code(const resolved_export& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_export>(e);
}

r_expr remove_dead_code(const resolved_call& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_call>(e);
}

r_expr remove_dead_code(const resolved_object& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_object>(e);
}

r_expr remove_dead_code(const resolved_let& e, const std::unordered_set<std::string>& dead_args) {
    if (dead_args.count(e.id_name())) {
        return remove_dead_code(e.body, dead_args);
    }
    return make_rexpr<resolved_let>(e.identifier, remove_dead_code(e.body, dead_args), e.type, e.loc);
}

r_expr remove_dead_code(const resolved_conditional& e,const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_conditional>(e);
}

r_expr remove_dead_code(const resolved_float& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_float>(e);
}

r_expr remove_dead_code(const resolved_int& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_int>(e);
}

r_expr remove_dead_code(const resolved_unary& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_unary>(e);
}

r_expr remove_dead_code(const resolved_binary& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_binary>(e);
}

r_expr remove_dead_code(const resolved_field_access& e, const std::unordered_set<std::string>& dead_args) {
    return make_rexpr<resolved_field_access>(e);
}

r_expr remove_dead_code(const r_expr& e, const std::unordered_set<std::string>& dead_args) {
    return std::visit([&](auto& c) {return remove_dead_code(c, dead_args);}, *e);
}

std::pair<r_expr, bool> eliminate_dead_code(const r_expr& e) {
    auto result = e;
    std::unordered_set<std::string> dead_code;
    find_dead_code(e, dead_code);
    if (!dead_code.empty()) result = remove_dead_code(e, dead_code);
    return {result, !dead_code.empty()};
}

} // namespace resolved_ir
} // namespace al