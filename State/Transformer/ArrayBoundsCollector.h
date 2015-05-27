//
// Created by ice-phoenix on 5/26/15.
//

#ifndef SANDBOX_ARRAYCOLLECTOR_H
#define SANDBOX_ARRAYCOLLECTOR_H

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class ArrayBoundsCollector : public Transformer<ArrayBoundsCollector> {

    using Base = Transformer<ArrayBoundsCollector>;

public:

    using ArrayBounds = std::unordered_map<Term::Ptr, Term::Set, TermHash, TermEquals>;

    ArrayBoundsCollector(FactoryNest FN);

    Predicate::Ptr transformPredicate(Predicate::Ptr pred);

    const ArrayBounds& getArrayBounds() const;

private:

    ArrayBounds arrayBounds;

};

} // namespace borealis

#endif //SANDBOX_ARRAYCOLLECTOR_H
