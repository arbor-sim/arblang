#pragma once
#include <sstream>

#include <arblang/pre_printer/printable_mechanism.hpp>

namespace al {
namespace resolved_ir {

std::stringstream print_header(const printable_mechanism& mech,
                               const std::string& cpp_namespace,
                               bool cpu = true,
                               bool gpu = false);

} // namespace resolved_ir
} // namespace al