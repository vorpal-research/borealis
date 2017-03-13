#include "State/Transformer/MarkEraser.h"

#include "Util/macros.h"

namespace borealis {

PredicateState::Ptr MarkEraser::transformBasic(BasicPredicateStatePtr basic) {
    return basic->filter(LAM(p, not llvm::isa<MarkPredicate>(p)));
}

} /* namespace borealis */


#include "Util/unmacros.h"
