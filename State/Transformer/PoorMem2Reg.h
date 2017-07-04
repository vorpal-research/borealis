#ifndef POOR_MEM_2_REG_H
#define POOR_MEM_2_REG_H

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class PoorMem2Reg: public Transformer<PoorMem2Reg> {

    using Base = Transformer;

    using Mapping = std::unordered_map<Term::Ptr, Term::Ptr, TermHash, TermEquals>;
    struct Memories {
        Mapping memory;
        Mapping bounds;
        std::unordered_map<std::string, Mapping> properties;
    };
    std::unordered_map<size_t, Memories> mapping;
    std::unordered_set<size_t> invalidatedMS;

    PoorMem2Reg(const PoorMem2Reg&) = default;

public:
    using Transformer::Transformer;

    PredicateState::Ptr transformChoice(PredicateStateChoicePtr choice);

    Predicate::Ptr transformStore(StorePredicatePtr store);
    Predicate::Ptr transformWriteBound(WriteBoundPredicatePtr wb);
    Predicate::Ptr transformWriteProperty(WritePropertyPredicatePtr wp);

    Predicate::Ptr transformEquality(EqualityPredicatePtr eq);
    Predicate::Ptr transformAlloca(AllocaPredicatePtr alloc);

    Term::Ptr transformLoadTerm(LoadTermPtr term);
    Term::Ptr transformBoundTerm(BoundTermPtr term);
    Term::Ptr transformReadPropertyTerm(ReadPropertyTermPtr term);
};

} /* namespace borealis */

#endif // POOR_MEM_2_REG_H
