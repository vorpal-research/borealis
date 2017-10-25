//
// Created by abdullin on 10/17/17.
//

#ifndef BOREALIS_PREDICATESTATEINTERPRETER_H
#define BOREALIS_PREDICATESTATEINTERPRETER_H

#include <llvm/Pass.h>

namespace borealis {

class PredicateStateInterpreter: public llvm::ModulePass {
public:

    static char ID;
    PredicateStateInterpreter();
    virtual ~PredicateStateInterpreter() = default;

    bool runOnModule(llvm::Module& M) override;
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
};

}   // namespace borealis

#endif //BOREALIS_PREDICATESTATEINTERPRETER_H
