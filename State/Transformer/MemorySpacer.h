#ifndef MEMORY_SPACER_H
#define MEMORY_SPACER_H

#include "State/Transformer/CachingTransformer.hpp"
#include "State/Transformer/LocalStensgaardAA.h"

namespace borealis {

class MemorySpacer : public borealis::CachingTransformer<MemorySpacer> {

    using Base = CachingTransformer<MemorySpacer>;

    LocalStensgaardAA aa;
    std::unordered_map<void*, size_t> indices;
    size_t currentIndex = 1;

    size_t getIndex(void* token);
    size_t getMemspace(Term::Ptr t);
public:

    MemorySpacer(FactoryNest FN, PredicateState::Ptr space);

    using Base::transformBase;
    Term::Ptr transformTerm(Term::Ptr t);

};

} // namespace borealis

#endif //MEMORY_SPACER_H
