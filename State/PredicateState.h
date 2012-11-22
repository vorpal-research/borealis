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

#include "Logging/logger.hpp"
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

    PredicateState addPredicate(Predicate::Ptr pred) const;

    std::pair<z3::expr, z3::expr> toZ3(Z3ExprFactory& z3ef) const;

    typedef std::list<Predicate::Ptr> Data;
    typedef Data::value_type DataEntry;
    typedef Data::const_iterator DataIterator;

    struct Hash {
        static size_t hash(const PredicateState& state) {
            size_t h = 17;
            for (const auto& e : state) {
                h = 3 * (size_t)e.get() + h;
            }
            return h;
        }

        size_t operator()(const PredicateState& state) const {
            return hash(state);
        }
    };

    typedef std::set<std::string> SortedData;

    DataIterator begin() const { return data.begin(); }
    DataIterator end() const { return data.end(); }
    bool empty() const { return data.empty(); }

    bool operator==(const PredicateState& other) const {
        return data == other.data;
    }

    SortedData sorted() const {
        SortedData res;
        for (auto& e : data) {
            res.insert(e->toString());
        }
        return res;
    }

private:

    Data data;

    void removeDependants(Predicate::DependeeSet dependees);

};

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateState& state);
logging::stream_t& operator<<(logging::stream_t& s, const PredicateState& state);

} /* namespace borealis */

#endif /* PREDICATESTATE_H_ */
