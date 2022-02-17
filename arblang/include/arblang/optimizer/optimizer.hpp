#pragma once

#include <iostream>
#include <string>
#include <unordered_set>

#include <arblang/optimizer/constant_fold.hpp>
#include <arblang/optimizer/copy_propagate.hpp>
#include <arblang/optimizer/cse.hpp>
#include <arblang/optimizer/eliminate_dead_code.hpp>

namespace al {
namespace resolved_ir {

class optimizer {
private:
    r_expr expression_;
    bool keep_optimizing_ = true;

public:
    optimizer(const r_expr& e): expression_(e) {}
    void start() {
        keep_optimizing_ = true;
    }
    void reset() {
        keep_optimizing_ = false;
    }
    void optimize() {
        keep_optimizing_ = false;
        auto result = cse(expression_);
        expression_ = result.first;
        keep_optimizing_ |= result.second;

//        std::cout << "--------------0------------" << std::endl;
//        std::cout << to_string(expression_) << std::endl << std::endl;

        result = constant_fold(expression_);
        expression_ = result.first;
        keep_optimizing_ |= result.second;

//        std::cout << "--------------1------------" << std::endl;
//        std::cout << to_string(expression_) << std::endl << std::endl;

        result = copy_propagate(expression_);
        expression_ = result.first;
        keep_optimizing_ |= result.second;

//        std::cout << "--------------2------------" << std::endl;
//        std::cout << to_string(expression_) << std::endl << std::endl;

        result = eliminate_dead_code(expression_);
        expression_ = result.first;
        keep_optimizing_ |= result.second;

//        std::cout << "--------------3------------" << std::endl;
//        std::cout << to_string(expression_) << std::endl << std::endl;
    }
    bool keep_optimizing() {
        return keep_optimizing_;
    }
    r_expr expression() {
        return expression_;
    }
};

} // namespace resolved_ir
} // namespace al