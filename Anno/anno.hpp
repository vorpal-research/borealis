/*
 * anno.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef ANNO_HPP
#define ANNO_HPP

#include "Anno/calculator.hpp"
#include "Anno/grammar.hpp"

namespace borealis {
namespace anno {
namespace calculator {

template<class Location = pegtl::ascii_location>
std::vector< command_type > parse_command(const std::string& command) {

    std::unique_ptr< command_type > ret{ new command_type() };
    expr_stack stack;
    std::vector< command_type > commands;
    auto cp = anno::calc_parse_string< annotated_commentary, Location >(command, stack, *ret, commands);

    if (cp.success) {
        return std::move(commands);
    } else {
        return std::vector< command_type >();
    }
}

template<class Location = pegtl::ascii_location>
expression_type parse_term(const std::string& term) {

    std::unique_ptr< command_type > ret{ new command_type() };
    expr_stack stack;
    std::vector< command_type > commands;
    auto cp = anno::calc_parse_string< read_expr, Location >(term, stack, *ret, commands);

    if (cp.success) {
        return stack.top();
    } else {
        return expression_type{};
    }
}

template<class Iter, class Location = pegtl::ascii_location>
std::vector< command_type > parse_command(const std::pair<Iter, Iter>& command) {

    std::unique_ptr< command_type > ret{ new command_type() };
    expr_stack stack;
    std::vector< command_type > commands;
    auto cp = anno::calc_parse_string< annotated_commentary, Location >(command, stack, *ret, commands);

    if (cp.success) {
        return std::move(commands);
    } else {
        return std::vector< command_type >();
    }
}

template<class Iter, class Location = pegtl::ascii_location>
expression_type parse_term(const std::pair<Iter, Iter>& term) {

    std::unique_ptr< command_type > ret{ new command_type() };
    expr_stack stack;
    std::vector< command_type > commands;
    auto cp = anno::calc_parse_string< read_expr, Location >(term, stack, *ret, commands);

    if (cp.success) {
        return stack.top();
    } else {
        return expression_type{};
    }
}

}   // namespace calculator
}   // namespace anno
}   // namespace borealis

#endif // ANNO_HPP
