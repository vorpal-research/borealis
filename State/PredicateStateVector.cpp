/*
 * PredicateStateVector.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: ice-phoenix
 */

#include "State/PredicateStateVector.h"

namespace borealis {

using borealis::util::view;

PredicateStateVector::PredicateStateVector() {}

PredicateStateVector::PredicateStateVector(const PredicateStateVector& psv) :
    data(psv.data) {
}

PredicateStateVector PredicateStateVector::addPredicate(Predicate::Ptr pred) const {
    PredicateStateVector res = PredicateStateVector();
    for (const auto& state : this->data) {
        res.data.insert(state.addPredicate(pred));
    }
    return res;
}

PredicateStateVector PredicateStateVector::merge(const PredicateStateVector& psv) const {
    PredicateStateVector res = PredicateStateVector(*this);
    for (const auto& state : psv.data) {
        res.data.insert(state);
    }
    return res;
}

PredicateStateVector PredicateStateVector::merge(const PredicateState& state) const {
    PredicateStateVector res = PredicateStateVector(*this);
    res.data.insert(state);
    return res;
}

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateStateVector& vec) {
    using borealis::util::streams::endl;
    s << '[';
    if (!vec.empty()) {
        auto iter = vec.begin();
        const PredicateState& el = *iter++;
        s << endl << "  " << el;
        for (const auto& e : view(iter, vec.end())) {
            s << ',' << endl << "  " << e;
        }
    }
    s << endl << ']';
    return s;
}

std::ostream& operator<<(std::ostream& s, const PredicateStateVector& vec) {
    using std::endl;
    s << '[';
    if (!vec.empty()) {
        auto iter = vec.begin();
        const PredicateState& el = *iter++;
        s << endl << "  " << el;
        for (const auto& e : view(iter, vec.end())) {
            s << ',' << endl << "  " << e;
        }
    }
    s << endl << ']';
    return s;
}

} /* namespace borealis */
