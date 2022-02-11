#include <string>
#include <unordered_set>

static std::string unique_local_name(std::unordered_set<std::string>& reserved, std::string const& prefix = "t") {
    for (int i = 0; ; ++i) {
        std::string name = prefix + std::to_string(i) + "_";
        if (reserved.insert(name).second) {
            return name;
        }
    }
}