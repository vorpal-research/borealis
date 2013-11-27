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

    typedef std::list<Predicate::Ptr> Data;

public:

    MK_COMMON_STATE_IMPL(BasicPredicateState);

    const Data& getData() const { return data; }

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const override;

    virtual PredicateState::Ptr addVisited(const llvm::Value* l) const override;
    virtual bool hasVisited(std::initializer_list<const llvm::Value*> ls) const override;
    virtual bool hasVisitedFrom(Locs& visited) const override;

    virtual Locs getVisited() const override;

    virtual PredicateState::Ptr map(Mapper m) const override;
    virtual PredicateState::Ptr filterByTypes(std::initializer_list<PredicateType> types) const override;
    virtual PredicateState::Ptr filter(Filterer f) const override;

    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(std::initializer_list<PredicateType> types) const override;
    virtual PredicateState::Ptr sliceOn(PredicateState::Ptr on) const override;
    virtual PredicateState::Ptr simplify() const override;

    virtual bool isEmpty() const override;

    virtual bool equals(const PredicateState* other) const override {
        if (auto* o = llvm::dyn_cast_or_null<Self>(other)) {
            return PredicateState::equals(other) &&
                    util::equal(data, o->data,
                        [](const Predicate::Ptr& a, const Predicate::Ptr& b) {
                            return *a == *b;
                        }
                    );
        } else return false;
    }

    virtual std::string toString() const override;
    virtual borealis::logging::logstream& dump(borealis::logging::logstream& s) const override;

private:

    Data data;
    Locs locs;

    BasicPredicateState();

    void addPredicateInPlace(Predicate::Ptr pred);
    void addVisitedInPlace(const llvm::Value* loc);
    void addVisitedInPlace(const Locs& locs);

};

template<class Impl>
struct SMTImpl<Impl, BasicPredicateState> {
    static Bool<Impl> doit(
            const BasicPredicateState* s,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        auto res = ef.getTrue();
        for (auto& v : s->getData()) {
            res = res && SMT<Impl>::doit(v, ef, ctx);
        }
        res = res && ctx->toSMT();

        return res;
    }
};

} /* namespace borealis */

#endif /* BASICPREDICATESTATE_H_ */
