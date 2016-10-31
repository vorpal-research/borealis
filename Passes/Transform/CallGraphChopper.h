//
// Created by belyaev on 10/27/16.
//

#ifndef CALLGRAPHCHOPPER_H
#define CALLGRAPHCHOPPER_H

#include <llvm/Pass.h>

namespace borealis {

class CallGraphChopper: public llvm::ModulePass {

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("callgraph-chopper")
#include "Util/unmacros.h"

    CallGraphChopper() : llvm::ModulePass(ID) {};
    virtual ~CallGraphChopper() {};

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnModule(llvm::Module&) override;
    virtual void print(llvm::raw_ostream& O, const llvm::Module* M) const override;

};

} /* namespace borealis */

#endif /* CALLGRAPHCHOPPER_H */
