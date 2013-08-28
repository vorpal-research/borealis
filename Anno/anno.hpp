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

}   // namespace calculator
}   // namespace anno
}   // namespace borealis

#endif // ANNO_HPP
