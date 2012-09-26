/*
 * util.h
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "llvm/Support/raw_ostream.h"

#include <string>

namespace llvm {
// copy the standard ostream behavior with functions
llvm::raw_ostream& operator<<(llvm::raw_ostream& ost, llvm::raw_ostream& (*op)(llvm::raw_ostream&));

std::string conditionToString(const int cond);
}

namespace borealis {
namespace util {
namespace streams {

// copy the standard ostream endl
llvm::raw_ostream& endl(llvm::raw_ostream& ost);

} // namespace streams
} // namespace util
} // namespace borealis

#include "util.hpp"

#endif /* UTIL_H_ */
