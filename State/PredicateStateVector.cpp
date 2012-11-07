/*
 * PredicateStateVector.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: ice-phoenix
 */

#include "PredicateStateVector.h"

namespace borealis {

using util::view;

PredicateStateVector::PredicateStateVector() {
}

PredicateStateVector::PredicateStateVector(bool) {
    data.insert(PredicateState());
}

PredicateStateVector::PredicateStateVector(const PredicateStateVector& psv) : data(psv.data) {
}

PredicateStateVector PredicateStateVector::addPredicate(const Predicate* pred) const {
    PredicateStateVector res = PredicateStateVector();
    for (const auto& state : this->data) {
        res.data.insert(state.addPredicate(pred));
    }
    return res;
}

PredicateStateVector PredicateStateVector::merge(const PredicateStateVector& psv) const {
    PredicateStateVector res = PredicateStateVector(psv);
    for (const auto& state : this->data) {
        res.data.insert(state);
    }
    return res;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateStateVector& vec) {
    s << '[';
    if (!vec.empty()) {
        auto iter = vec.begin();
        const PredicateState& el = *iter++;
        s << el;
        for (const auto& e : view(iter, vec.end())) {
            s << ',' << e;
        }
    }
    s << ']';
    return s;
}

logging::stream_t& operator<<(logging::stream_t& s, const PredicateStateVector& vec) {
    s << '[';
    if (!vec.empty()) {
        auto iter = vec.begin();
        const PredicateState& el = *iter++;
        s << el;
        for (const auto& e : view(iter, vec.end())) {
            s << ',' << e;
        }
    }
    s << ']';
    return s;
}

} /* namespace borealis */
