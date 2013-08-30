/*
 * sayonara.hpp
 *
 *  Created on: Aug 30, 2013
 *      Author: ice-phoenix
 */

#ifndef SAYONARA_HPP_
#define SAYONARA_HPP_

#include <llvm/Support/ManagedStatic.h>

#include <cstddef>
#include <string>

#include "debugbreak/debugbreak.h"

namespace borealis {
namespace util {

template<typename RetTy = void>
RetTy sayonara(const std::string& file, int line, const std::string& where, const std::string& reason) {
    using namespace borealis::logging;

    debug_break();

    errs() << file << ":" << std::to_string(line) << endl
            << "\t" << where << endl
            << "\t" << reason << endl;
    std::exit(EXIT_FAILURE);
}

} // namespace util
} // namespace borealis

#endif /* SAYONARA_HPP_ */
