//
// Created by ice-phoenix on 4/29/15.
//

#ifndef SANDBOX_RANDOMSLICER_H
#define SANDBOX_RANDOMSLICER_H

#include <random>

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class RandomSlicer : public Transformer<RandomSlicer> {

    using Base = Transformer<RandomSlicer>;

public:

    RandomSlicer();

    using Base::transform;
    PredicateState::Ptr transform(PredicateState::Ptr ps);
    using Base::transformBase;
    Predicate::Ptr transformBase(Predicate::Ptr p);

private:

    std::random_device rd;
    std::mt19937 mtr;

    unsigned int size;

    double getNextRandom();

};

} // namespace borealis

#endif //SANDBOX_RANDOMSLICER_H
