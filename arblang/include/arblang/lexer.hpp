#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <arblang/token.hpp>

namespace al{
class lexer_impl;

struct state {
    const char* line_start;
    const char* stream;
    unsigned line;
    token tok;
};

class lexer {
public:
    lexer(const char* begin);

    const token& current();
    const token& next(unsigned n=1);
    token peek(unsigned n=1);
    state save();
    void restore(state);
    ~lexer();

private:
    std::unique_ptr<lexer_impl> impl_;
};

} // namespace al