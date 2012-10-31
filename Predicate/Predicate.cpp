/*
 * Predicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate.h"

namespace llvm {
llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const borealis::Predicate& p) {
	s << p.toString();
	return s;
}
} // namespace llvm

namespace borealis {

Predicate::Predicate() : Predicate(PredicateType::STATE) {}

Predicate::Predicate(const PredicateType type) : type(type) {}

} // namespace borealis
