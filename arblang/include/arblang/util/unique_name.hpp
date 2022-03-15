#pragma once

#include <string>
#include <unordered_set>

static std::string unique_local_name(std::unordered_set<std::string>& reserved, std::string const& prefix) {
    for (int i = 0; ; ++i) {
        std::string name = "_" + prefix + std::to_string(i);
        if (reserved.insert(name).second) {
            return name;
        }
    }
}