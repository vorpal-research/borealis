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

PredicateState::PredicateState() {
}

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

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateState& state) {
	s << '(';
	if (!state.empty()) {
		auto iter = state.begin();
		const PredicateState::DataEntry& el = *iter++;
		s << *(el.second);
		std::for_each(iter, state.end(), [&s](const PredicateState::DataEntry& e){
			s << ',' << *(e.second);
		});
	}
	s << ')';
	return s;
}

} /* namespace borealis */
