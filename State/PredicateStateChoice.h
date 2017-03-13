/*
 * PredicateStateChoice.h
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATECHOICE_H_
#define PREDICATESTATECHOICE_H_

#include <vector>

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

    using Choices = std::vector<PredicateState::Ptr>;

public:

    MK_COMMON_STATE_IMPL(PredicateStateChoice);

    const Choices& getChoices() const;

    virtual PredicateState::Ptr addPredicate(Predicate::Ptr pred) const override;

    virtual PredicateState::Ptr addVisited(const Locus& locus) const override;
    virtual bool hasVisited(std::initializer_list<Locus> loci) const override;
    virtual bool hasVisitedFrom(Loci& visited) const override;

    virtual Loci getVisited() const override;

    virtual PredicateState::Ptr fmap(FMapper f) const override;

    virtual std::pair<PredicateState::Ptr, PredicateState::Ptr> splitByTypes(
            std::initializer_list<PredicateType> types) const override;
    virtual PredicateState::Ptr sliceOn(PredicateState::Ptr on) const override;;
    virtual PredicateState::Ptr simplify() const override;;

    virtual bool isEmpty() const override;;
    virtual unsigned int size() const override;

    virtual bool equals(const PredicateState* other) const override;

    virtual std::string toString() const override;
    virtual borealis::logging::logstream& dump(borealis::logging::logstream& s) const override;

private:

    Choices choices;

    PredicateStateChoice(const Choices& choices);
    PredicateStateChoice(Choices&& choices);

    SelfPtr fmap_(FMapper f) const;

};

template<class Impl>
struct SMTImpl<Impl, PredicateStateChoice> {
    static Bool<Impl> doit(
            const PredicateStateChoice* s,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx,
            bool pathMode = false) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        auto&& res = ef.getFalse();
        std::vector<std::pair<Bool, ExecutionContext>> memories;
        memories.reserve(s->getChoices().size());

        static borealis::config::BoolConfigEntry freeVarChoicesSetting("analysis", "freevar-choices");
        auto freeVarChoices = freeVarChoicesSetting.get(false);
        static int uniqueness = 0;
        auto pfix = tfm::format("$ext%d", ++uniqueness);

        for (auto&& choice : s->getChoices()) {
            ExecutionContext choiceCtx(*ctx);

            auto&& z3state = SMT<Impl>::doit(choice, ef, &choiceCtx, pathMode);

            if(not freeVarChoices) {
                auto&& path = choice->filterByTypes({PredicateType::PATH})->simplify();
                res = res || z3state;

                if(!pathMode) {
                    auto&& z3path = *path == *choice ? z3state : SMT<Impl>::doit(path, ef, &choiceCtx, true);
                    memories.push_back({z3path.simplify(), choiceCtx});
                }
            } else {
                res = res || (z3state && choiceCtx.externalizeEverythingMutable(pfix));
                memories.push_back({ef.getTrue(), choiceCtx});
            }
        }

        if(not freeVarChoices) {
            if(not pathMode) ctx->switchOn("choice", memories);
        } else {
            ctx->externalSwitchOn(memories, pfix);
        }


        return res;
    }
};

struct PredicateStateChoiceExtractor {

    auto unapply(PredicateState::Ptr t) const -> functional_hell::matchers::storage_t<decltype(std::declval<PredicateStateChoice>().getChoices())> {
        if (auto&& tt = llvm::dyn_cast<PredicateStateChoice>(t)) {
            return functional_hell::matchers::make_storage(tt->getChoices());
        } else {
            return {};
        }
    }

};

static auto $PredicateStateChoice = functional_hell::matchers::make_pattern(PredicateStateChoiceExtractor());


} /* namespace borealis */

#endif /* PREDICATESTATECHOICE_H_ */
