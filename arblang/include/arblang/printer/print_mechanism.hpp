#pragma once
#include <sstream>

#include <arblang/pre_printer/printable_mechanism.hpp>

namespace al {
namespace resolved_ir {

std::stringstream print_mechanism(const printable_mechanism& mech, const std::string& cpp_namespace);

} // namespace resolved_ir
} // namespace al