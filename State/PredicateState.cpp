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

PredicateState PredicateState::addPredicate(const Predicate* pred) {
	PredicateState res = PredicateState(*this);
	data[pred->getKey()] = pred;
	return res;
}

PredicateState PredicateState::merge(const PredicateState& state) {
	PredicateState res = PredicateState(*this);
	for_each(state, [this, &res](const DataEntry& entry){
		data[entry.first] = entry.second;
	});
	return res;
}

PredicateState::~PredicateState() {
	// TODO
}

} /* namespace borealis */
