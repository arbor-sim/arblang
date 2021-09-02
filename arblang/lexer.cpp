#include <cstdio>
#include <iostream>
#include <string>

#include <arblang/lexer.hpp>
#include <arblang/token.hpp>

namespace al{
// helpers for identifying character types
inline bool is_plusminus(char c) {
    return (c=='+' || c=='-');
}

class lexer_impl {
    const char* line_start_;
    const char* stream_;
    unsigned line_;
    token token_;

public:

    lexer_impl(const char* begin): line_start_(begin), stream_(begin), line_(0) {
        parse(); // prepare the first token
    }

    // Return the current token in the stream.
    const token& current() {
        return token_;
    }

    const token& next(unsigned n=1) {
        while (n--) parse();
        return token_;
    }

    token peek(unsigned n) {
        // Save state.
        auto ls = line_start_;
        auto st = stream_;
        auto l  = line_;
        auto t  = token_;

        // Advance n tokens.
        next(n);

        // Restore state.
        std::swap(t, token_);
        line_ = l;
        line_start_ = ls;
        stream_ = st;

        return t;
    }

private:
    src_location loc() const {
        return src_location(line_+1, stream_-line_start_+1);
    }

    bool empty() const {
        return *stream_ == '\0';
    }

    // Consumes characters in the stream until end of stream or a new line.
    // Assumes that the current location is the `#` that starts the comment.
    void eat_comment() {
        while (!empty() && *stream_!='\n') {
            ++stream_;
        }
    }

    // Look ahead n characters in the input stream.
    // If peek to or past the end of the stream return '\0'.
    char peek_char(int n) {
        const char* c = stream_;
        while (*c && n--) ++c;
        return *c;
    }

    char character() {
        return *stream_++;
    }

    token number() {
        using namespace std::string_literals;
        auto start = loc();
        std::string str;
        char c = *stream_;

        // Start counting the number of points in the number.
        auto num_point = (c=='.' ? 1 : 0);
        auto uses_scientific_notation = false;

        str += c;
        ++stream_;
        while(1) {
            c = *stream_;
            if (std::isdigit(c)) {
                str += c;
                ++stream_;
            }
            else if (c=='.') {
                if (++num_point>1) {
                    // Can't have more than one '.' in a number
                    return {start, tok::error, "Unexpected '.'"s};
                }
                str += c;
                ++stream_;
                if (uses_scientific_notation) {
                    // Can't have a '.' in the mantissa
                    return {start, tok::error, "Unexpected '.'"s};
                }
            }
            else if (!uses_scientific_notation && (c=='e' || c=='E')) {
                auto c0 = peek_char(1);
                auto c1 = peek_char(2);
                if (std::isdigit(c0) || (is_plusminus(c0) && std::isdigit(c1))) {
                    uses_scientific_notation = true;
                    str += c;
                    stream_++;
                    // Consume the next char if +/-
                    if (is_plusminus(*stream_)) {
                        str += *stream_++;
                    }
                }
                else {
                    // the 'e' or 'E' is the beginning of a new token
                    break;
                }
            }
            else {
                break;
            }
        }
        const bool is_float = uses_scientific_notation || num_point>0;
        return {start, (is_float? tok::floatpt: tok::integer), std::move(str)};
    }

    // Scan identifier from stream
    token symbol() {
        using namespace std::string_literals;
        auto start = loc();
        std::string identifier;
        char c = *stream_;

        // Assert that current position is at the start of an identifier
        if(!(std::isalpha(c) || c == '_')) {
            return {start, tok::error, "Expected identifier start."s};
        }
        identifier+=c;
        ++stream_;
        while(1) {
            c = *stream_;
            if((std::isalnum(c) || c == '_' || c == '\'')) {
                identifier+=c;
                ++stream_;
            }
            else {
                break;
            }
        }
        return {start, token::tokenize(identifier).value_or(tok::identifier), std::move(identifier)};
    }

    void parse() {
        using namespace std::string_literals;
        while(!empty()) {
            switch(*stream_) {
                // end of file
                case 0:   // end of string
                    token_ = {loc(), tok::eof, "eof"};
                    return ;

                // white space
                case ' ' :
                case '\t':
                case '\v':
                case '\f':
                    ++stream_;
                    continue;   // skip to next character

                // new line
                case '\n':
                    line_++;
                    ++stream_;
                    line_start_ = stream_;
                    continue;   // skip to next line

                // carriage return (windows new line)
                case '\r':
                    ++stream_;
                    if(*stream_ != '\n') {
                        token_ = {loc(), tok::error, "Expected new line after carriage return (bad line ending)"};
                        return;
                    }
                    continue; // catch the new line on the next pass

                case '#':
                    eat_comment();
                    continue;
                case '(':
                    token_ = {loc(), tok::lparen, {character()}};
                    return;
                case ')':
                    token_ = {loc(), tok::rparen, {character()}};
                    return;
                case '{':
                    token_ = {loc(), tok::lbrace, {character()}};
                    return;
                case '}':
                    token_ = {loc(), tok::rbrace, {character()}};
                    return;
                case '0' ... '9':
                    token_ = number();
                    return;
                case '.':
                    if (std::isdigit(peek_char(1))) {
                        token_ = number();
                    } else {
                        token_ = {loc(), tok::dot, {character()}};
                    }
                    return;
                // identifier or keyword
                case 'a' ... 'z':
                case 'A' ... 'Z':
                    token_ = symbol();
                    return;
                case '=': {
                    if(peek_char(1)=='=') {
                        token_ = {loc(), tok::equality, {character(), character()}};
                    }
                    else {
                        token_ = {loc(), tok::eq, {character()}};
                    }
                    return;
                }
                case '!': {
                    if(peek_char(1)=='=') {
                        token_ = {loc(), tok::ne, {character(), character()}};
                    }
                    else {
                        token_ = {loc(), tok::lnot, {character()}};
                    }
                    return;
                }
                case '+':
                    token_ = {loc(), tok::plus, {character()}};
                    return;
                case '-':
                    if (peek_char(1)=='>') {
                        token_ = {loc(), tok::ret, {character(), character()}};
                    }
                    else {
                        token_ = {loc(), tok::minus, {character()}};
                    }
                    return;
                case '/':
                    token_ = {loc(), tok::divide, {character()}};
                    return;
                case '*':
                    token_ = {loc(), tok::times, {character()}};
                    return;
                case '^':
                    token_ = {loc(), tok::pow, {character()}};
                    return;
                // comparison binary operators and reaction
                case '<': {
                    if (peek_char(1)=='-' && peek_char(2)=='>') {
                        std::string s = {character(), character(), character()};
                        token_ = {loc(), tok::arrow, s};
                    }
                    else if (peek_char(1)=='=') {
                        token_ = {loc(), tok::le, {character(), character()}};
                    }
                    else {
                        token_ = {loc(), tok::lt, {character()}};
                    }
                    return;
                }
                case '>': {
                    if (peek_char(1)=='=') {
                        token_ = {loc(), tok::ge, {character(), character()}};
                    }
                    else {
                        token_ = {loc(), tok::gt, {character()}};
                    }
                    return;
                }
                case '&': {
                    if (peek_char(1)!='&') {
                        token_ = {loc(), tok::error, "Expected & in a pair."};
                        return;
                    }
                    token_ = {loc(), tok::land, {character(), character()}};
                    return;
                }
                case '|': {
                    if (peek_char(1)!='|') {
                        token_ = {loc(), tok::error, "Expected | in a pair."};
                        return;
                    }
                    token_ = {loc(), tok::lor, {character(), character()}};
                    return;
                }
                case ',':
                    token_ = {loc(), tok::comma, {character()}};
                    return;
                case ';':
                    token_ = {loc(), tok::semicolon, {character()}};
                    return;
                case ':':
                    token_ = {loc(), tok::colon, {character()}};
                    return;
                default:
                    token_ = {loc(), tok::error, std::string("Unexpected character '")+character()+"'"};
                    return;
            }
        }
        // return the token
        if (!empty()) {
            token_ = {loc(), tok::error, "Internal lexer error: expected end of input, please open a bug report"s};
            return;
        }
        token_ = {loc(), tok::eof, "eof"s};
    }

};

lexer::lexer(const char* begin):
    impl_(new lexer_impl(begin))
{}

const token& lexer::current() {
    return impl_->current();
}

const token& lexer::next(unsigned n) {
    return impl_->next(n);
}

token lexer::peek(unsigned n) {
    return impl_->peek(n);
}

lexer::~lexer() = default;

} // namespace al