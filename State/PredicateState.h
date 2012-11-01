/*
 * PredicateState.h
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATE_H_
#define PREDICATESTATE_H_

#include <llvm/Support/raw_ostream.h>
#include <llvm/Value.h>

#include <unordered_map>

#include "Predicate/Predicate.h"
#include "Solver/Z3ExprFactory.h"
#include "Util/util.h"

namespace borealis {

class PredicateState {

public:

	PredicateState();
	PredicateState(const PredicateState& state);
	PredicateState(PredicateState&& state);

	const PredicateState& operator=(const PredicateState& state);
	const PredicateState& operator=(PredicateState&& state);

	PredicateState addPredicate(const Predicate* pred) const;
	PredicateState merge(const PredicateState& state) const;

	std::pair<z3::expr, z3::expr> toZ3(Z3ExprFactory& z3ef) const;

	typedef std::unordered_map<Predicate::Key, const Predicate*, Predicate::KeyHash> Data;
	typedef Data::value_type DataEntry;
	typedef Data::const_iterator DataIterator;

	DataIterator begin() const { return data.begin(); }
	DataIterator end() const { return data.end(); }
	bool empty() const { return data.empty(); }

	bool operator==(const PredicateState& other) const {
		return data == other.data;
	}

	struct Hash {
	public:
		size_t operator()(const PredicateState& ps) const {
			size_t res = 17;
			for (const auto& entry : ps) {
				res = res * Predicate::KeyHash::hash(entry.first) + 33;
			}
			return res;
		}
	};

private:

	Data data;

	void removeDependants(Predicate::DependeeSet dependees);

};

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateState& state);

} /* namespace borealis */

#endif /* PREDICATESTATE_H_ */
