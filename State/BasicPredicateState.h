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

    static bool classof(const Self* /* ps */) {
        return true;
    }

    static bool classof(const PredicateState* ps) {
        return ps->getPredicateStateTypeId() == type_id<Self>();
    }

    virtual bool isEmpty() const override;

    virtual bool equals(const PredicateState* other) const override {
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

    virtual std::string toString() const override;
    virtual borealis::logging::logstream& dump(borealis::logging::logstream& s) const override;

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
