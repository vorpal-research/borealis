#ifndef MARK_ERASER_H
#define MARK_ERASER_H


#include "State/Transformer/CachingTransformer.hpp"

namespace borealis {

class MarkEraser: public CachingTransformer<MarkEraser> {

    using Base = CachingTransformer<MarkEraser>;

public:
    MarkEraser(FactoryNest FN): Base(FN) {}

    PredicateState::Ptr transformBasic(BasicPredicateStatePtr basic);

};

template<class Logical>
Logical eraseMarks(FactoryNest FN, Logical ll) {
    return MarkEraser(FN).transform(ll);
}

} /* namespace borealis */

#endif // MARK_ERASER_H
