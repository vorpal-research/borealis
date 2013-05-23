/*
 * PredicateStateChain.h
 *
 *  Created on: Apr 30, 2013
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATECHAIN_H_
#define PREDICATESTATECHAIN_H_

#include "State/PredicateState.h"

namespace borealis {

class PredicateStateChain :
        public PredicateState,
        public std::enable_shared_from_this<PredicateStateChain> {

public:

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const;
    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* pctx = nullptr) const;

    virtual PredicateState::Ptr addVisited(const llvm::Value* loc) const;
    virtual bool hasVisited(std::initializer_list<const llvm::Value*> locs) const;

    virtual PredicateState::Ptr map(Mapper m) const;
    virtual PredicateState::Ptr filterByTypes(std::initializer_list<PredicateType> types) const;
    virtual PredicateState::Ptr filter(Filterer f) const;
    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(std::initializer_list<PredicateType> types) const;

    virtual bool isEmpty() const;

    static bool classof(const PredicateStateChain* /* ps */) {
        return true;
    }

    static bool classof(const PredicateState* ps) {
        return ps->getPredicateStateTypeId() == type_id<PredicateStateChain>();
    }

    virtual bool equals(const PredicateState* other) const {
        if (this == other) return true;

        if (auto* o = llvm::dyn_cast_or_null<PredicateStateChain>(other)) {
            return *this->base == *o->base &&
                    *this->curr == *o->curr;
        } else {
            return false;
        }
    }

    virtual std::string toString() const;

    friend class PredicateStateFactory;

private:

    PredicateState::Ptr base;
    PredicateState::Ptr curr;

    PredicateStateChain();
    PredicateStateChain(PredicateState::Ptr base, PredicateState::Ptr curr);
    PredicateStateChain(const PredicateStateChain& state) = default;
    PredicateStateChain(PredicateStateChain&& state) = default;

};

} /* namespace borealis */

#endif /* PREDICATESTATECHAIN_H_ */
