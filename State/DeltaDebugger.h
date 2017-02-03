//
// Created by ice-phoenix on 4/29/15.
//

#ifndef SANDBOX_DELTADEBUGGER_H
#define SANDBOX_DELTADEBUGGER_H

#include "SMT/Z3/Z3.h"
#include "State/PredicateState.h"
#include "State/Transformer/RandomSlicer.h"

namespace borealis {

template<class Predicate>
class DeltaDebugger {
    Predicate pred;
    size_t tries;

public:
    DeltaDebugger(Predicate pred, size_t tries = 1000): pred(pred), tries(tries) {}

    PredicateState::Ptr reduce(PredicateState::Ptr ps) {
        auto&& curr = ps;

        for (auto&& i = 0_size; i < tries; ++i) {
            auto&& opt = RandomSlicer().transform(curr);
            dbgs() << "Delta-debugging from " << curr->size() << " to " << opt->size() << endl;

            if (pred(opt)) {
                if (opt->size() < curr->size()) {
                    curr = opt;
                }
            }
        }

        return curr;
    }
};

template<class Predicate>
DeltaDebugger<Predicate> makeDeltaDebugger(Predicate pred, size_t tries = 1000) {
    return {pred, tries};
}

} // namespace borealis

#endif //SANDBOX_DELTADEBUGGER_H
