/*
 * sayonara.cpp
 *
 *  Created on: Sep 9, 2013
 *      Author: belyaev
 */

#include <debugbreak/debugbreak.h>
#include <cstdlib>

#include "Logging/logger.hpp"
#include "Util/util.h"

namespace borealis {
namespace util {

namespace {
    using diehandler = void(*)(const char*);

    [[noreturn]] void defaultDie(const char* m) {
        errs() << m;

        debug_break();
        std::exit(EXIT_FAILURE);
    }
}

void ondie(const char* m, diehandler hndl = nullptr) {
    static diehandler inner = defaultDie;
    if (!hndl) inner(m);
    inner = hndl;
}

[[noreturn]] void diediedie(const char* m) {
    ondie(m);
    throw 0;
}

} // namespace util
} // namespace borealis
