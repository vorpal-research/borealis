/*
 * BasicPredicateState.h
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#ifndef BASICPREDICATESTATE_H_
#define BASICPREDICATESTATE_H_

#include <llvm/Value.h>

#include <list>
#include <unordered_set>

#include "Logging/logger.hpp"
#include "State/PredicateState.h"
#include "Util/util.h"

namespace borealis {

class BasicPredicateState :
        public PredicateState {

    typedef BasicPredicateState Self;
    typedef std::unique_ptr<Self> SelfPtr;

    typedef std::list<Predicate::Ptr> Data;
    typedef std::unordered_set<const llvm::Value*> Locations;

public:

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const;
    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* pctx = nullptr) const;

    virtual PredicateState::Ptr addVisited(const llvm::Value* loc) const;
    virtual bool hasVisited(std::initializer_list<const llvm::Value*> locs) const;

    virtual PredicateState::Ptr map(Mapper m) const;
    virtual PredicateState::Ptr filterByTypes(std::initializer_list<PredicateType> types) const;
    virtual PredicateState::Ptr filter(Filterer f) const;
    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(std::initializer_list<PredicateType> types) const;

    virtual PredicateState::Ptr sliceOn(PredicateState::Ptr base) const;

    virtual PredicateState::Ptr simplify() const;

    static bool classof(const Self* /* ps */) {
        return true;
    }

    static bool classof(const PredicateState* ps) {
        return ps->getPredicateStateTypeId() == type_id<Self>();
    }

    virtual bool isEmpty() const;

    virtual bool equals(const PredicateState* other) const {
        if (this == other) return true;

        if (auto* o = llvm::dyn_cast_or_null<Self>(other)) {
            return std::equal(data.begin(), data.end(), o->data.begin(),
                [](const Predicate::Ptr& a, const Predicate::Ptr& b) {
                    return *a == *b;
                });
        } else {
            return false;
        }
    }

    virtual std::string toString() const;

    friend class PredicateStateFactory;

private:

    Data data;
    Locations locs;

    BasicPredicateState();
    BasicPredicateState(const Self& state) = default;
    BasicPredicateState(Self&& state) = default;

    void addPredicateInPlace(Predicate::Ptr pred);
    void addVisitedInPlace(const llvm::Value* loc);
    void addVisitedInPlace(const Locations& locs);

};

} /* namespace borealis */

#endif /* BASICPREDICATESTATE_H_ */
