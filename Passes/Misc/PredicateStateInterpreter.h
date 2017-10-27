//
// Created by abdullin on 10/17/17.
//

#ifndef BOREALIS_PREDICATESTATEINTERPRETER_H
#define BOREALIS_PREDICATESTATEINTERPRETER_H

#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {

class PredicateStateInterpreter: public ProxyFunctionPass, public ShouldBeModularized {
public:

    static char ID;
    PredicateStateInterpreter();
    PredicateStateInterpreter(llvm::Pass* pass);
    virtual ~PredicateStateInterpreter() = default;

    bool runOnFunction(llvm::Function& F) override;
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
};

}   // namespace borealis

#endif //BOREALIS_PREDICATESTATEINTERPRETER_H
