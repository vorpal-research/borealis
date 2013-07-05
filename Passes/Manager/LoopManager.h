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

#include "Logging/logger.hpp"

namespace borealis {

class LoopManager :
        public llvm::FunctionPass,
        public borealis::logging::ClassLevelLogging<LoopManager> {

public:

    typedef std::unordered_map<llvm::BasicBlock*, unsigned> Data;

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("loop")
#include "Util/unmacros.h"

    LoopManager();
    virtual bool runOnFunction(llvm::Function&) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~LoopManager();

    virtual void print(llvm::raw_ostream&, const llvm::Module*) const override;

    unsigned getUnrollCount(llvm::Loop* L) const;

private:

    Data data;

};

} /* namespace borealis */

#endif /* LOOPMANAGER_H_ */
