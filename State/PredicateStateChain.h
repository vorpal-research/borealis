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
        public PredicateState {

    typedef PredicateStateChain Self;
    typedef std::unique_ptr<Self> SelfPtr;

public:

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const override;
    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* pctx = nullptr) const override;

    virtual PredicateState::Ptr addVisited(const llvm::Value* loc) const override;
    virtual bool hasVisited(std::initializer_list<const llvm::Value*> locs) const override;

    virtual PredicateState::Ptr map(Mapper m) const override;
    virtual PredicateState::Ptr filterByTypes(std::initializer_list<PredicateType> types) const override;
    virtual PredicateState::Ptr filter(Filterer f) const override;
    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(std::initializer_list<PredicateType> types) const override;

    virtual PredicateState::Ptr sliceOn(PredicateState::Ptr base) const override;

    virtual PredicateState::Ptr simplify() const override;

    virtual bool isEmpty() const override;

    static bool classof(const Self* /* ps */) {
        return true;
    }

    static bool classof(const PredicateState* ps) {
        return ps->getPredicateStateTypeId() == type_id<Self>();
    }

    virtual bool equals(const PredicateState* other) const override {
        if (this == other) return true;

        if (auto* o = llvm::dyn_cast_or_null<Self>(other)) {
            return *this->base == *o->base &&
                    *this->curr == *o->curr;
        } else {
            return false;
        }
    }

    virtual std::string toString() const override;
    virtual borealis::logging::logstream& dump(borealis::logging::logstream& s) const override;

    friend class PredicateStateFactory;

private:

    PredicateState::Ptr base;
    PredicateState::Ptr curr;

    PredicateStateChain(PredicateState::Ptr base, PredicateState::Ptr curr);
    PredicateStateChain(const Self& state) = default;
    PredicateStateChain(Self&& state) = default;

};

} /* namespace borealis */

#endif /* PREDICATESTATECHAIN_H_ */
