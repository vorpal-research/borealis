/*
 * PredicateState.h
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATE_H_
#define PREDICATESTATE_H_

#include "llvm/Value.h"

#include <unordered_map>

#include "../Predicate/Predicate.h"
#include "../util.h"

namespace borealis {

using util::for_each;

class PredicateState {

public:

	PredicateState(const PredicateState& state);

	PredicateState addPredicate(const Predicate* pred) const;
	PredicateState merge(const PredicateState& state) const;

	virtual ~PredicateState();

	typedef std::unordered_map<Predicate::Key, const Predicate*, Predicate::KeyHash> Data;
	typedef Data::value_type DataEntry;
	typedef Data::const_iterator DataIterator;

	DataIterator begin() const { return data.begin(); }
	DataIterator end() const { return data.end(); }

	bool operator==(const PredicateState& other) const {
		return data == other.data;
	}

	struct Hash {
	public:
		size_t operator()(const PredicateState& ps) const {
			size_t res = 17;
			for_each(ps, [&res](const DataEntry& entry){
				res = res * Predicate::KeyHash::hash(entry.first) + 33;
			});
			return res;
		}
	};

private:

	Data data;

};

} /* namespace borealis */

#endif /* PREDICATESTATE_H_ */
