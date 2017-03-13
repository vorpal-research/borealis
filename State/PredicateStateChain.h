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

    PredicateState::Ptr getBase() const;
    PredicateState::Ptr getCurr() const;

    PredicateState::Ptr swapBase(PredicateState::Ptr newBase) const;
    PredicateState::Ptr swapCurr(PredicateState::Ptr newCurr) const;

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const override;

    virtual PredicateState::Ptr addVisited(const Locus& locus) const override;
    virtual bool hasVisited(std::initializer_list<Locus> loci) const override;
    virtual bool hasVisitedFrom(Loci& visited) const override;

    virtual Loci getVisited() const override;

    virtual PredicateState::Ptr fmap(FMapper f) const override;
    virtual PredicateState::Ptr reverse() const override;

    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(
            std::initializer_list<PredicateType> types) const override;
    virtual PredicateState::Ptr sliceOn(PredicateState::Ptr on) const override;
    virtual PredicateState::Ptr simplify() const override;

    virtual bool isEmpty() const override;
    virtual unsigned int size() const override;

    virtual bool equals(const PredicateState* other) const override;

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
            ExecutionContext<Impl>* ctx,
            bool pathMode = false) {
        TRACE_FUNC;

        auto res = ef.getTrue();
        res = res && SMT<Impl>::doit(s->getBase(), ef, ctx, pathMode);
        res = res && SMT<Impl>::doit(s->getCurr(), ef, ctx, pathMode);
        return res;
    }
};

struct PredicateStateChainExtractor {

    auto unapply(PredicateState::Ptr t) const -> functional_hell::matchers::storage_t<PredicateState::Ptr, PredicateState::Ptr> {
        if (auto&& tt = llvm::dyn_cast<PredicateStateChain>(t)) {
            return functional_hell::matchers::make_storage(tt->getBase(), tt->getCurr());
        } else {
            return {};
        }
    }

};

static auto $PredicateStateChain = functional_hell::matchers::make_pattern(PredicateStateChainExtractor());

} /* namespace borealis */

#endif /* PREDICATESTATECHAIN_H_ */
