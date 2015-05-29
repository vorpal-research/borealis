/*
 * command.hpp
 *
 *  Created on: Apr 2, 2012
 *      Author: belyaev
 */

#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include <algorithm>
#include <iostream>
#include <list>
#include <string>

#include "Anno/production.h"
#include "Util/util.h"

namespace borealis {
namespace anno {

struct command {
    std::string name_;
    std::string meta_;
    std::list<prod_t> args_;
};

template<class Streamer>
Streamer& operator<<(Streamer& ost, const command& com) {
    ost << com.name_;
    if(!com.meta_.empty()) ost << "[[" << com.meta_ << "]]";

    if (!com.args_.empty()) {
        ost << "(" << *borealis::util::head(com.args_);
        for (const prod_t& arg : borealis::util::tail(com.args_)) {
            ost <<  "," << *arg;
        }
        ost << ")";
    }

    // this is generally fucked up
    return static_cast<Streamer&>(ost);
}

} // namespace anno
} // namespace borealis

#endif /* COMMAND_HPP_ */
