/*
 * Predicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATE_H_
#define PREDICATE_H_

#include "llvm/Support/raw_ostream.h"
#include "llvm/Value.h"
#include "z3/z3++.h"

#include <typeindex>
#include <tuple>

#include "../util.h"

namespace borealis {

enum PredicateType {
	PATH,
	STATE
};

class Predicate {

public:

	typedef std::pair<size_t, const llvm::Value*> Key;

	struct KeyHash {
	public:
		static size_t hash(const Key& k) {
			return k.first ^ (size_t)k.second;
		}

		size_t operator()(const Key& k) const {
			return hash(k);
		}
	};

	Predicate();
	Predicate(const PredicateType type);
	virtual ~Predicate() {};
	virtual Key getKey() const = 0;

	std::string toString() const {
		return asString;
	}

	PredicateType getType() const {
		return type;
	}

	virtual z3::expr toZ3(z3::context& ctx) const = 0;

protected:

	std::string asString;
	PredicateType type;

};

} /* namespace borealis */

namespace llvm {
llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const borealis::Predicate& p);
}

#endif /* PREDICATE_H_ */
