//
// Created by ice-phoenix on 4/29/15.
//

#ifndef SANDBOX_DELTADEBUGGER_H
#define SANDBOX_DELTADEBUGGER_H

#include "SMT/Z3/Z3.h"
#include "State/PredicateState.h"

namespace borealis {

class DeltaDebugger {

    USING_SMT_IMPL(Z3);

public:

    DeltaDebugger(ExprFactory& ef);

    PredicateState::Ptr reduce(PredicateState::Ptr ps);

private:

    ExprFactory& z3ef;
    unsigned long long memoryStart;
    unsigned long long memoryEnd;

    z3::tactic tactics(unsigned int timeout);

};

} // namespace borealis

#endif //SANDBOX_DELTADEBUGGER_H
