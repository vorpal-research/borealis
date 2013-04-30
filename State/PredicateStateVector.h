/*
 * PredicateStateVector.h
 *
 *  Created on: Oct 4, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEVECTOR_H_
#define PREDICATESTATEVECTOR_H_

#include <algorithm>
#include <unordered_set>

#include "Predicate/Predicate.h"
#include "State/PredicateState.h"

namespace borealis {

class PredicateStateVector {

public:

    PredicateStateVector();
    PredicateStateVector(const PredicateStateVector& psv) = default;

    PredicateStateVector merge(const PredicateStateVector& psv) const;
    PredicateStateVector merge(PredicateState::Ptr state) const;

    typedef std::unordered_set<PredicateState::Ptr> Data;
    typedef Data::value_type DataEntry;
    typedef Data::const_iterator DataIterator;
    typedef Data::size_type DataSizeType;

    DataIterator begin() const { return data.begin(); }
    DataIterator end() const { return data.end(); }
    bool empty() const { return data.empty(); }
    DataSizeType size() const { return data.size(); }

    bool operator==(const PredicateStateVector& other) const {
        return data == other.data;
    }

    bool operator!=(const PredicateStateVector& other) const {
        return data != other.data;
    }

    template<class Condition>
    PredicateStateVector remove_if(Condition cond) const {
        PredicateStateVector res = PredicateStateVector(*this);
        for (auto iter = res.data.begin(); iter != res.data.end(); ) {
            if (cond(*iter)) {
                iter = res.data.erase(iter);
            } else {
                ++iter;
            }
        }
        return res;
    }

private:

    Data data;

};

std::ostream& operator<<(std::ostream& s, const PredicateStateVector& vec);

} /* namespace borealis */

#endif /* PREDICATESTATEVECTOR_H_ */
