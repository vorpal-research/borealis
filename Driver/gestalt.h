/*
 * gestalt.h
 *
 *  Created on: Aug 27, 2013
 *      Author: belyaev
 */

#ifndef GESTALT_H_
#define GESTALT_H_

#include "Logging/logger.hpp"

namespace borealis {
namespace driver {

enum {
    OK                           = 0x0000,
    E_ILLEGAL_COMPILER_OPTIONS   = 0x0001,
    E_GATHER_COMMENTS            = 0x0002,
    E_EMIT_LLVM                  = 0x0003,
    E_CLANG_INVOKE               = 0x0004
};

class gestalt: public borealis::logging::ObjectLevelLogging<gestalt> {
public:
    gestalt(const std::string& loggingDomain):
        borealis::logging::ObjectLevelLogging<gestalt>(loggingDomain) {}

    int main(int argc, const char** argv);

    ~gestalt();
};

} /* namespace driver */
} /* namespace borealis */

#endif /* GESTALT_H_ */
