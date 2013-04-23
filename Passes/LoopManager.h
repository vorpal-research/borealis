/*
 * LoopManager.h
 *
 *  Created on: Mar 29, 2013
 *      Author: belyaev
 */

#ifndef LOOPMANAGER_H_
#define LOOPMANAGER_H_

#include <llvm/Pass.h>

#include <unordered_map>

#include "Util/util.h"

namespace borealis {

class LoopManager: public llvm::FunctionPass {

public:

    typedef std::unordered_map<llvm::BasicBlock*, unsigned> Data;

    static char ID;

    LoopManager();
    virtual bool runOnFunction(llvm::Function&);
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual ~LoopManager();
    virtual void print(llvm::raw_ostream&, const llvm::Module*) const;

    unsigned getUnrollCount(llvm::Loop* L) const;

private:

    Data data;

};

} /* namespace borealis */

#endif /* LOOPMANAGER_H_ */
