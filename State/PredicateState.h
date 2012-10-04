/*
 * PredicateState.h
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATE_H_
#define PREDICATESTATE_H_

#include "llvm/Value.h"

#include <unordered_map>

#include "../Predicate/Predicate.h"

namespace borealis {

class PredicateState {

public:

	PredicateState(const PredicateState& state);

	PredicateState addPredicate(const Predicate* pred);
	PredicateState merge(const PredicateState& state);

	virtual ~PredicateState();

	typedef std::unordered_map<Predicate::Key, const Predicate*, Predicate::KeyHash> Data;
	typedef Data::value_type DataEntry;
	typedef Data::const_iterator DataIterator;

	DataIterator begin() const { return data.begin(); }
	DataIterator end() const { return data.end(); }

private:

	Data data;

};

} /* namespace borealis */

#endif /* PREDICATESTATE_H_ */
