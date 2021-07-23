#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <arblang/error.hpp>
#include <arblang/token.hpp>

namespace al{
class lexer_impl;
class lexer {
public:
    lexer(const char* begin);

    const token& current();
    const token& next(unsigned n=1);
    token peek(unsigned n=1);

    ~lexer();

private:
    std::unique_ptr<lexer_impl> impl_;
};

} // namespace al