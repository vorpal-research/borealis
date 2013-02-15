/*
 * SCCPass.h
 *
 *  Created on: Feb 7, 2013
 *      Author: ice-phoenix
 */

#ifndef SCCPASS_H_
#define SCCPASS_H_

#include <llvm/Analysis/CallGraph.h>
#include <llvm/Pass.h>

#include <vector>

namespace borealis {

class SCCPass : public llvm::ModulePass {

public:

    typedef std::vector<llvm::CallGraphNode*> CallGraphSCC;
    typedef CallGraphSCC::value_type CallGraphSCCNode;

    SCCPass(char& ID);
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual bool runOnModule(llvm::Module&);
    virtual ~SCCPass();

    virtual bool runOnSCC(CallGraphSCC& SCC) = 0;
};

} /* namespace borealis */

#endif /* SCCPASS_H_ */