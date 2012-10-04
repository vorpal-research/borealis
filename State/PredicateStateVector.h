/*
 * PredicateStateVector.h
 *
 *  Created on: Oct 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEVECTOR_H_
#define PREDICATESTATEVECTOR_H_

#include <unordered_set>

#include "PredicateState.h"
#include "../Predicate/Predicate.h"

namespace borealis {

class PredicateStateVector {

public:

	PredicateStateVector();
	PredicateStateVector(const PredicateStateVector& psv);

	PredicateStateVector addPredicate(const Predicate* pred) const;
	PredicateStateVector merge(const PredicateStateVector& psv) const;

	virtual ~PredicateStateVector();

	typedef std::unordered_set<PredicateState, PredicateState::Hash> Data;
	typedef Data::value_type DataEntry;
	typedef Data::const_iterator DataIterator;

	DataIterator begin() const { return data.begin(); }
	DataIterator end() const { return data.end(); }

	bool operator==(const PredicateStateVector& other) const {
		return data == other.data;
	}

private:

	Data data;

};

} /* namespace borealis */

#endif /* PREDICATESTATEVECTOR_H_ */
