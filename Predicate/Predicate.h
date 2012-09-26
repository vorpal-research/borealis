/*
 * Predicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATE_H_
#define PREDICATE_H_

#include "llvm/Support/raw_ostream.h"

#include <string>

namespace borealis {

class Predicate {
public:
	virtual ~Predicate() {};
	virtual std::string toString() const = 0;
};

} /* namespace borealis */

namespace llvm {
llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const borealis::Predicate& p);
}

#endif /* PREDICATE_H_ */
