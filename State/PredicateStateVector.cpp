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

PredicateStateVector PredicateStateVector::merge(const PredicateStateVector& psv) const {
    PredicateStateVector res = PredicateStateVector(*this);
    for (auto& state : psv.data) {
        res.data.insert(state);
    }
    return res;
}

PredicateStateVector PredicateStateVector::merge(PredicateState::Ptr state) const {
    PredicateStateVector res = PredicateStateVector(*this);
    res.data.insert(state);
    return res;
}

std::ostream& operator<<(std::ostream& s, const PredicateStateVector& vec) {
    using std::endl;

    s << '[';
    if (!vec.empty()) {
        auto iter = vec.begin();
        auto& el = *iter++;
        s << endl << "  " << el;
        for (auto& e : view(iter, vec.end())) {
            s << ',' << endl << "  " << e;
        }
    }
    s << endl << ']';
    return s;
}

} /* namespace borealis */
