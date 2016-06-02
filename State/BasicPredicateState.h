/*
 * BasicPredicateState.h
 *
 *  Created on: Oct 3, 2012
 *      Author: ice-phoenix
 */

#ifndef BASICPREDICATESTATE_H_
#define BASICPREDICATESTATE_H_

#include <vector>

#include "State/PredicateState.h"

namespace borealis {

/** protobuf -> State/BasicPredicateState.proto
import "State/PredicateState.proto";
import "Predicate/Predicate.proto";

package borealis.proto;

message BasicPredicateState {
    extend borealis.proto.PredicateState {
        optional BasicPredicateState ext = 16;
    }

    repeated Predicate data = 1;
}

**/
class BasicPredicateState :
        public PredicateState {

    using Data = std::vector<Predicate::Ptr>;

public:

    MK_COMMON_STATE_IMPL(BasicPredicateState);

    const Data& getData() const;

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const override;

    virtual PredicateState::Ptr addVisited(const Locus& locus) const override;
    virtual bool hasVisited(std::initializer_list<Locus> loci) const override;
    virtual bool hasVisitedFrom(Loci& visited) const override;

    virtual Loci getVisited() const override;

    virtual PredicateState::Ptr fmap(FMapper) const override;

    virtual PredicateState::Ptr map(Mapper m) const override;
    virtual PredicateState::Ptr filterByTypes(std::initializer_list<PredicateType> types) const override;
    virtual PredicateState::Ptr filter(Filterer f) const override;
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

    Data data;
    Loci loci;

    BasicPredicateState();
    BasicPredicateState(size_t size);
    BasicPredicateState(const Data& data);
    BasicPredicateState(Data&& data);

    void addPredicateInPlace(Predicate::Ptr pred);
    void addVisitedInPlace(const Locus& locus);
    void addVisitedInPlace(const Loci& loci_);

};

template<class Impl>
struct SMTImpl<Impl, BasicPredicateState> {
    static Bool<Impl> doit(
            const BasicPredicateState* s,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx,
            bool = false) {
        TRACE_FUNC;

        auto&& res = ef.getTrue();
        for (auto&& v : s->getData()) {
            res = res && SMT<Impl>::doit(v, ef, ctx);
        }
        res = res && ctx->toSMT();

        return res;
    }
};

} /* namespace borealis */

#endif /* BASICPREDICATESTATE_H_ */
