/*
 * PredicateState.cpp
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#include "PredicateState.h"

#include "../util.h"

namespace borealis {

using util::for_each;

PredicateState::PredicateState(const PredicateState& state) :
	data(state.data) {
}

PredicateState PredicateState::addPredicate(const Predicate* pred) const {
	PredicateState res = PredicateState(*this);
	res.data[pred->getKey()] = pred;
	return res;
}

PredicateState PredicateState::merge(const PredicateState& state) const {
	PredicateState res = PredicateState(*this);
	for_each(state, [this, &res](const DataEntry& entry){
		res.data[entry.first] = entry.second;
	});
	return res;
}

PredicateState::~PredicateState() {
	// TODO
}

} /* namespace borealis */
