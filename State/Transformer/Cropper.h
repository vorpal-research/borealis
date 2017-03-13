#ifndef CROPPER_H
#define CROPPER_H

#include "State/PredicateState.def"
#include "State/Transformer/Transformer.hpp"

namespace borealis {

template<class Checker>
class Cropper : public borealis::Transformer<Cropper<Checker>> {

    using Base = borealis::Transformer<Cropper<Checker>>;

public:
    Cropper(FactoryNest FN, Checker checker): Base(FN), checker(checker) {}

    // XXX: Why do we have to do this???
#define HANDLE_STATE(NAME, CLASS) \
    using CLASS ## Ptr = typename Base::CLASS ## Ptr;
#include "State/PredicateState.def"
#undef HANDLE_STATE

    PredicateState::Ptr transformBasic(BasicPredicateStatePtr p) {
        if(crop) return p;

        std::vector<Predicate::Ptr> collect;
        bool changed = false;
        for(auto&& pred : p->getData()) {
            collect.push_back(pred);
            if(checker(pred))  {
                changed = true;
                break;
            }
        }

        if(changed) {
            crop = true;
            auto res = this->FN.State->Basic(collect);
            for(auto&& whatever : p->getVisited()) res->addVisited(whatever);
            return res;
        }
        return p;
    }

    PredicateState::Ptr transformChain(PredicateStateChainPtr c) {
        if(crop) return c;

        auto rebase = this->transform(c->getBase());
        if(crop) return rebase;
        auto recurr = this->transform(c->getCurr());
        if(crop) return this->FN.State->Chain(rebase, recurr);

        return c;
    }

    PredicateState::Ptr transformChoice(PredicateStateChoicePtr c) {
        if(crop) return c;

        std::vector<PredicateState::Ptr> croppedChoice;
        bool encounteredCrop = false;
        for(auto&& sub : c->getChoices()) {
            crop = false;

            auto cropped = Base::transform(sub);
            if(crop) {
                croppedChoice.push_back(cropped);
                encounteredCrop = true;
            }
        }

        if(not encounteredCrop) {
            return c;
        }
        crop = true;
        return this->FN.State->Choice(std::move(croppedChoice));
    }

private:

    Checker checker;
    bool crop = false;

};

template<class Checker>
PredicateState::Ptr cropStateOn(PredicateState::Ptr state, FactoryNest FN, Checker checker) {
    return Cropper<Checker>(FN, checker).transform(state);
}

} // namespace borealis

#endif // CROPPER_H
