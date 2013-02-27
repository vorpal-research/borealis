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

#include <functional>
#include <initializer_list>
#include <list>
#include <unordered_set>

#include "Logging/logger.hpp"
#include "Predicate/Predicate.h"
#include "Solver/Z3ExprFactory.h"
#include "Util/util.h"

namespace borealis {

class PredicateState {

public:

    PredicateState();
    PredicateState(Predicate::Ptr p);
    PredicateState(const PredicateState& state);
    PredicateState(PredicateState&& state);

    PredicateState& operator=(const PredicateState& state);
    PredicateState& operator=(PredicateState&& state);

    PredicateState addPredicate(Predicate::Ptr pred) const;
    PredicateState addAll(const PredicateState& state) const;

    PredicateState addVisited(const llvm::Instruction* location) const;

    template<class H, class ...T>
    bool hasVisited(H* loc, T&... rest) const {
        if (!borealis::util::contains(visited, loc)) return false;
        return hasVisited(rest...);
    }

    template<class H, class ...T>
    bool hasVisited(H& loc, T&... rest) const {
        if (!borealis::util::contains(visited, &loc)) return false;
        return hasVisited(rest...);
    }

    template<class H>
    bool hasVisited(H* loc) const {
        return borealis::util::contains(visited, loc);
    }

    template<class H>
    bool hasVisited(H& loc) const {
        return borealis::util::contains(visited, &loc);
    }

    bool isUnreachable() const;

    logic::Bool toZ3(Z3ExprFactory& z3ef) const;

    typedef std::list<Predicate::Ptr> Data;
    typedef Data::value_type DataEntry;
    typedef Data::const_iterator DataIterator;

    typedef std::unordered_set<const llvm::Instruction*> Locations;

    struct Hash {
        static size_t hash(const PredicateState& state) {
            size_t h = 17;
            for (const auto& e : state) {
                h = 3 * h + e->hashCode();
            }
            return h;
        }

        size_t operator()(const PredicateState& state) const {
            return hash(state);
        }
    };

    DataIterator begin() const { return data.begin(); }
    DataIterator end() const { return data.end(); }
    bool empty() const { return data.empty(); }

    PredicateState map(std::function<Predicate::Ptr(Predicate::Ptr)> f) const {
        PredicateState res;
        for (auto& p : data) {
            res = res.addPredicate(f(p));
        }
        return res;
    }

    PredicateState filter(std::function<bool(Predicate::Ptr)> f = DEFAULT) const {
        PredicateState res;
        for (auto& p : data) {
            if (f(p)) res = res.addPredicate(p);
        }
        return res;
    }

    static bool PATH(Predicate::Ptr p) {
        return p->getType() == PredicateType::PATH; }
    static bool STATE(Predicate::Ptr p) {
        return p->getType() == PredicateType::STATE ||
               p->getType() == PredicateType::ENSURES; }
    static bool DEFAULT(Predicate::Ptr p) {
        return p->getType() == PredicateType::PATH ||
               p->getType() == PredicateType::STATE ||
               p->getType() == PredicateType::ENSURES; }

    bool operator==(const PredicateState& other) const {
        if (this == &other) return true;

        return std::equal(begin(), end(), other.begin(),
            [](const Predicate::Ptr& a, const Predicate::Ptr& b) {
                return *a == *b;
            });
    }

private:

    Data data;

    Locations visited;

};

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const PredicateState& state);
std::ostream& operator<<(std::ostream& s, const PredicateState& state);

////////////////////////////////////////////////////////////////////////////////
//
// PredicateState operators
//
////////////////////////////////////////////////////////////////////////////////

const PredicateState operator&&(const PredicateState& state, Predicate::Ptr p);
const PredicateState operator+(const PredicateState& state, Predicate::Ptr p);

const PredicateState operator&&(const PredicateState& a, const PredicateState& b);
const PredicateState operator+(const PredicateState& a, const PredicateState& b);

} /* namespace borealis */

#endif /* PREDICATESTATE_H_ */
