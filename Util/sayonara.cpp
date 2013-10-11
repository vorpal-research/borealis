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

#include "Util/macros.h"

namespace borealis {
namespace util {

namespace {
    using diehandler = void(*)(const char*);

    void defaultDie(const char* m) {
        errs() << m;

        debug_break();
    }
}

void ondie(const char* m, diehandler hndl = nullptr) {
    static diehandler inner = defaultDie;
    if (!hndl) inner(m);
    inner = hndl;
}

NORETURN void diediedie(const char* m) {
    ondie(m);
    std::exit(EXIT_FAILURE);
}

#include "Util/unmacros.h"

} // namespace util
} // namespace borealis
