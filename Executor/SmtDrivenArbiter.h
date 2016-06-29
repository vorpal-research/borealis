//
// Created by belyaev on 4/7/15.
//

#ifndef SMT_DRIVEN_ARBITER_H
#define SMT_DRIVEN_ARBITER_H

#include "Executor/Arbiter.h"
#include "SMT/Result.h"
#include "Util/slottracker.h"

namespace borealis {

class SmtDrivenArbiter: public Arbiter {

    FactoryNest FN;
    smt::SatResult model;

public:
    SmtDrivenArbiter(const llvm::DataLayout* DL, SlotTracker* ST, const smt::SatResult& model):
        FN(DL, ST), model(model) {}

    virtual llvm::GenericValue map(llvm::Value* val);
};

} /* namespace borealis */

#endif /* SMT_DRIVEN_ARBITER_H */
