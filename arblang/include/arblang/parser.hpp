#include <string>
#include <vector>

#include <arblang/lexer.hpp>
#include <arblang/raw_expressions.hpp>

namespace al {
using namespace raw_ir;

class parser: lexer {
public:
    parser(std::string const&);
    void parse();

    const std::vector<expr>& modules() {return modules_;};

private:
    expr parse_module();
    expr parse_parameter();
    expr parse_constant();
    expr parse_record();
    expr parse_function();
    expr parse_import();

    std::vector<expr> modules_;
};
} // namespace al