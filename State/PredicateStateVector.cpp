/*
 * PredicateStateVector.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: ice-phoenix
 */

#include "PredicateStateVector.h"

namespace borealis {

PredicateStateVector::PredicateStateVector() {
}

PredicateStateVector::PredicateStateVector(const PredicateStateVector& psv) :
		data(psv.data) {
}

PredicateStateVector PredicateStateVector::addPredicate(const Predicate* pred) const {
	PredicateStateVector res = PredicateStateVector();
	for_each(this->data, [pred, &res](const DataEntry& state) {
		res.data.insert(state.addPredicate(pred));
	});
	return res;
}

PredicateStateVector PredicateStateVector::merge(const PredicateStateVector& psv) const {
	PredicateStateVector res = PredicateStateVector(psv);
	for_each(this->data, [&res](const DataEntry& state) {
		res.data.insert(state);
	});
	return res;
}

PredicateStateVector::~PredicateStateVector() {
	// TODO
}

} /* namespace borealis */
