/*
 * Predicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate.h"

namespace llvm {
raw_ostream& operator<<(raw_ostream& s, const borealis::Predicate& p) {
	s << p.toString();
	return s;
}
} /* namespace llvm */
