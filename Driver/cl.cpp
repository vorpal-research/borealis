/*
 * cl.cpp
 *
 *  Created on: Aug 26, 2013
 *      Author: belyaev
 */

#include "Driver/cl.h"

namespace borealis {
namespace driver {

template<class Streamer>
Streamer& operator<<(Streamer& str, const CommandLine& cl) {
    if (cl.empty()) {
        str << "<empty command line invocation>";
    } else {
        str << util::head(cl.args);
        for (const auto& arg : util::tail(cl.args)) {
            str << " " << arg;
        }
    }
    // this is generally fucked up
    return static_cast<Streamer&>(str);
}

std::ostream& operator<<(std::ostream& s, const CommandLine& cl) {
    return operator<< <std::ostream> (s, cl);
}

} /* namespace driver */
} /* namespace borealis */
