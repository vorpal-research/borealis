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

#include "PredicateState.h"
#include "Predicate/Predicate.h"

namespace borealis {

class PredicateStateVector {

public:

	PredicateStateVector();
	PredicateStateVector(bool _);
	PredicateStateVector(const PredicateStateVector& psv);

	PredicateStateVector addPredicate(const Predicate* pred) const;
	PredicateStateVector merge(const PredicateStateVector& psv) const;

	typedef std::unordered_set<PredicateState, PredicateState::Hash> Data;
	typedef Data::value_type DataEntry;
	typedef Data::const_iterator DataIterator;

	DataIterator begin() const { return data.begin(); }
	DataIterator end() const { return data.end(); }
	bool empty() const { return data.empty(); }

	bool operator==(const PredicateStateVector& other) const {
		return data == other.data;
	}

	bool operator!=(const PredicateStateVector& other) const {
		return data != other.data;
	}

    template<class Condition>
    PredicateStateVector remove_if(Condition cond) const {
        PredicateStateVector res = PredicateStateVector(*this);
        for (auto iter = res.data.begin(); iter != res.data.end(); )
        {
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

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateStateVector& vec);

} /* namespace borealis */

#endif /* PREDICATESTATEVECTOR_H_ */
