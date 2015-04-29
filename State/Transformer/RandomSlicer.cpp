//
// Created by ice-phoenix on 4/29/15.
//

#include "State/Transformer/RandomSlicer.h"

namespace borealis {

RandomSlicer::RandomSlicer() : Base(nullptr), rd(), mtr(rd()) {}

double RandomSlicer::getNextRandom() {
    return std::generate_canonical<double, 10>(mtr);
}

PredicateState::Ptr RandomSlicer::transform(PredicateState::Ptr ps) {
    size = ps->size();
    return Base::transform(ps)->filter([](auto&& p) { return !!p; });
}

Predicate::Ptr RandomSlicer::transformBase(Predicate::Ptr p) {
    if (getNextRandom() < (2.0 / size)) {
        return nullptr;
    } else {
        return p;
    }
}

} // namespace borealis
