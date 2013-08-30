/*
 * PredicateStateChoice.h
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATECHOICE_H_
#define PREDICATESTATECHOICE_H_

#include "Protobuf/Gen/State/PredicateStateChoice.pb.h"
#include "State/PredicateState.h"

namespace borealis {

/** protobuf -> State/PredicateStateChoice.proto
import "State/PredicateState.proto";

package borealis.proto;

message PredicateStateChoice {
    extend borealis.proto.PredicateState {
        optional PredicateStateChoice ext = 18;
    }

    repeated PredicateState choices = 1;
}

**/
class PredicateStateChoice :
        public PredicateState {

public:

    MK_COMMON_STATE_IMPL(PredicateStateChoice);

    const std::vector<PredicateState::Ptr> getChoices() const { return choices; }

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const override;

    virtual PredicateState::Ptr addVisited(const llvm::Value* loc) const override;
    virtual bool hasVisited(std::initializer_list<const llvm::Value*> locs) const override;
    virtual bool hasVisitedFrom(Locs& visited) const override;

    virtual Locs getVisited() const override;

    virtual PredicateState::Ptr fmap(FMapper f) const override;

    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(std::initializer_list<PredicateType> types) const override;
    virtual PredicateState::Ptr sliceOn(PredicateState::Ptr base) const override;;
    virtual PredicateState::Ptr simplify() const override;;

    virtual bool isEmpty() const override;;

    virtual bool equals(const PredicateState* other) const override {
        if (auto* o = llvm::dyn_cast_or_null<Self>(other)) {
            return PredicateState::equals(other) &&
                    std::equal(choices.begin(), choices.end(), o->choices.begin(),
                        [](PredicateState::Ptr a, PredicateState::Ptr b) {
                            return *a == *b;
                        }
                    );
        } else return false;
    }

    virtual std::string toString() const override;
    virtual borealis::logging::logstream& dump(borealis::logging::logstream& s) const override;

private:

    std::vector<PredicateState::Ptr> choices;

    PredicateStateChoice(const std::vector<PredicateState::Ptr>& choices);
    PredicateStateChoice(std::vector<PredicateState::Ptr>&& choices);

    SelfPtr fmap_(FMapper f) const;

};

template<class Impl>
struct SMTImpl<Impl, PredicateStateChoice> {
    static Bool<Impl> doit(
            const PredicateStateChoice* s,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        auto res = ef.getFalse();
        std::vector<std::pair<Bool, ExecutionContext>> memories;
        memories.reserve(s->getChoices().size());

        for (const auto& choice : s->getChoices()) {
            ExecutionContext choiceCtx(*ctx);

            auto path = choice->filterByTypes({PredicateType::PATH});
            auto z3state = SMT<Impl>::doit(choice, ef, &choiceCtx);
            auto z3path = SMT<Impl>::doit(path, ef, &choiceCtx);

            res = res || z3state;
            memories.push_back({z3path, choiceCtx});
        }

        ctx->switchOn("choice", memories);

        return res;
    }
};

} /* namespace borealis */

#endif /* PREDICATESTATECHOICE_H_ */
