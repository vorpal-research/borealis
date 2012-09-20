/*
 * output_util.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#include "llvm/Support/raw_ostream.h"

using llvm::raw_ostream;

namespace llvm {
// copy the standard ostream behavior with functions
raw_ostream& operator<<(raw_ostream& ost, raw_ostream& (*op)(raw_ostream&)) {
	return op(ost);
}
}

namespace borealis {
namespace util {
namespace streams {

// copy the standard ostream endl
llvm::raw_ostream& endl(llvm::raw_ostream& ost) {
	ost << '\n';
	ost.flush();
	return ost;
}

} // namespace streams
} // namespace util
} // namespace borealis
