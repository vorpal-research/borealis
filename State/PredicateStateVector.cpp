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

PredicateStateVector::PredicateStateVector(bool) {
	data.insert(PredicateState());
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

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateStateVector& vec) {
	s << '[';
	if (!vec.empty()) {
		auto iter = vec.begin();
		const PredicateState& el = *iter++;
		s << el;
		std::for_each(iter, vec.end(), [&s](const PredicateState& e){
			s << ',' << e;
		});
	}
	s << ']';
	return s;
}

} /* namespace borealis */
