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

/** protobuf -> State/PredicateStateChain.proto
import "State/PredicateState.proto";

package borealis.proto;

message PredicateStateChain {
    extend borealis.proto.PredicateState {
        optional PredicateStateChain ext = 17;
    }

    optional PredicateState base = 1;
    optional PredicateState curr = 2;
}

**/
class PredicateStateChain :
        public PredicateState {

public:

    MK_COMMON_STATE_IMPL(PredicateStateChain);

    PredicateState::Ptr getBase() const { return base; }
    PredicateState::Ptr getCurr() const { return curr; }

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const override;

    virtual PredicateState::Ptr addVisited(const Locus& l) const override;
    virtual bool hasVisited(std::initializer_list<Locus> ls) const override;
    virtual bool hasVisitedFrom(Locs& visited) const override;

    virtual Locs getVisited() const override;

    virtual PredicateState::Ptr fmap(FMapper f) const override;

    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(std::initializer_list<PredicateType> types) const override;
    virtual PredicateState::Ptr sliceOn(PredicateState::Ptr on) const override;
    virtual PredicateState::Ptr simplify() const override;

    virtual bool isEmpty() const override;

    virtual bool equals(const PredicateState* other) const override {
        if (auto* o = llvm::dyn_cast_or_null<Self>(other)) {
            return PredicateState::equals(other) &&
                    *this->base == *o->base &&
                    *this->curr == *o->curr;
        } else return false;
    }

    virtual std::string toString() const override;
    virtual borealis::logging::logstream& dump(borealis::logging::logstream& s) const override;

private:

    PredicateState::Ptr base;
    PredicateState::Ptr curr;

    PredicateStateChain(PredicateState::Ptr base, PredicateState::Ptr curr);

    SelfPtr fmap_(FMapper f) const;

};

template<class Impl>
struct SMTImpl<Impl, PredicateStateChain> {
    static Bool<Impl> doit(
            const PredicateStateChain* s,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        auto res = ef.getTrue();
        res = res && SMT<Impl>::doit(s->getBase(), ef, ctx);
        res = res && SMT<Impl>::doit(s->getCurr(), ef, ctx);
        return res;
    }
};

} /* namespace borealis */

#endif /* PREDICATESTATECHAIN_H_ */
