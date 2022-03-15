#include <string>
#include <unordered_map>

#include <fmt/core.h>

#include <arblang/pre_printer/get_read_arguments.hpp>

namespace al {
namespace resolved_ir {

void read_arguments(const resolved_record_alias& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_record_alias at "
                             "this stage in the compilation (after resolution).");
}
void read_arguments(const resolved_constant& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_constant at "
                             "this stage in the compilation (after optimization).");
}

void read_arguments(const resolved_function& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_function at "
                             "this stage in the compilation (after inlining).");
}

void read_arguments(const resolved_call& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_call at "
                             "this stage in the compilation (after inlining).");
}

void read_arguments(const resolved_state& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_state at "
                             "this stage in the compilation (during printing prep).");
}

void read_arguments(const resolved_bind& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_bind at "
                             "this stage in the compilation (during printing prep).");
}

void read_arguments(const resolved_export& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_export at "
                             "this stage in the compilation (during printing prep).");
}

void read_arguments(const resolved_field_access& e, std::vector<std::string>& vec) {
    throw std::runtime_error("Internal compiler error, didn't expect a resolved_export at "
                             "this stage in the compilation (during printing prep, after "
                             "simplification).");
}

void read_arguments(const resolved_parameter& e, std::vector<std::string>& vec) {
    read_arguments(e.value, vec);
}

void read_arguments(const resolved_initial& e, std::vector<std::string>& vec) {
    read_arguments(e.value, vec);
}

void read_arguments(const resolved_evolve& e, std::vector<std::string>& vec) {
    read_arguments(e.value, vec);
}

void read_arguments(const resolved_effect& e, std::vector<std::string>& vec) {
    read_arguments(e.value, vec);
}

void read_arguments(const resolved_argument& e, std::vector<std::string>& vec) {
    vec.push_back(e.name);
}

void read_arguments(const resolved_variable& e, std::vector<std::string>& vec) {}

void read_arguments(const resolved_object& e, std::vector<std::string>& vec) {
    for (const auto& a: e.field_values()) {
        read_arguments(a, vec);
    }
}

void read_arguments(const resolved_let& e, std::vector<std::string>& vec) {
    read_arguments(e.id_value(), vec);
    read_arguments(e.body, vec);
}

void read_arguments(const resolved_conditional& e, std::vector<std::string>& vec) {
    read_arguments(e.condition, vec);
    read_arguments(e.value_true, vec);
    read_arguments(e.value_false, vec);
}

void read_arguments(const resolved_float& e, std::vector<std::string>& vec) {}

void read_arguments(const resolved_int& e, std::vector<std::string>& vec) {}

void read_arguments(const resolved_unary& e, std::vector<std::string>& vec) {
    read_arguments(e.arg, vec);
}

void read_arguments(const resolved_binary& e, std::vector<std::string>& vec) {
    read_arguments(e.lhs, vec);
    read_arguments(e.rhs, vec);
}

void read_arguments(const r_expr& e, std::vector<std::string>& vec) {
    return std::visit([&](auto&& c){return read_arguments(c, vec);}, *e);
}

} // namespace resolved_ir
} // namespace al