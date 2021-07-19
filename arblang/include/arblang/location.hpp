#pragma once

#include <ostream>

namespace al{
struct location {
    int line;
    int column;

    location(): location(1, 1) {}
    location(int ln, int col): line(ln), column(col) {}
};

inline std::ostream& operator<< (std::ostream& os, location const& loc) {
    return os << "location(line " << loc.line << ", col " << loc.column << ")";
}
} // namespace al