#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <arblang/location.hpp>
#include <arblang/error.hpp>
#include <arblang/token.hpp>

namespace al{
enum class lexer_status {
    error,  // lexer has encounterd a problem
    happy   // lexer is in a good place
};

enum class associativity_kind {
    left,
    right
};

// Class implementing the lexer
// Takes a range of characters as input parameters
class lexer {
public:
    lexer(const char* begin, const char* end):
        begin_(begin),
        end_(end),
        current_(begin),
        line_(begin),
        location_()
    {
        if(begin_>end_) {
            throw std::out_of_range("Lexer(begin, end) : begin>end");
        }
        binop_prec_init();
    }

    lexer(const std::vector<char>& v): lexer(v.data(), v.data()+v.size()) {}

    lexer(std::string const& s): buffer_(s.data(), s.data()+s.size()+1) {
        begin_   = buffer_.data();
        end_     = buffer_.data() + buffer_.size();
        current_ = begin_;
        line_    = begin_;
        binop_prec_init();
    }

    // Get the next token
    token parse();

    void get_token() {
        token_ = parse();
    }

    // Return the next token in the stream without advancing the current position
    token peek();

    // Look for `t` until new line or eof without advancing the current position, return true if found
    bool search_to_eol(tok const& t);

    // Scan a number from the stream
    token number();

    // Scan an identifier string from the stream
    std::string identifier();

    // Scan a character from the stream
    char character();

    location loc() {return location_;}

    lexer_status status() {return status_;}

    const std::string& error_message() {return error_string_;};

    static int binop_precedence(tok tok);
    static associativity_kind operator_associativity(tok token);
protected:
    // buffer used for short-lived parsers
    std::vector<char> buffer_;

    // generate lookup tables (hash maps) for keywords
    void keywords_init();
    void token_strings_init();
    void binop_prec_init();

    // helper for determining if an identifier string matches a keyword
    tok get_identifier_type(std::string const& identifier);

    const char *begin_, *end_;// pointer to start and 1 past the end of the buffer
    const char *current_;     // pointer to current character
    const char *line_;        // pointer to start of current line
    location location_;  // current location (line,column) in buffer

    lexer_status status_ = lexer_status::happy;
    std::string error_string_;

    token token_;

    // Binary operator precedence
    static std::unordered_map<tok, int> binop_prec_;
};
} // namespace al