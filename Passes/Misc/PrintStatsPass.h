/*
 * PrintStatsPass.h
 *
 *  Created on: Jan 22, 2014
 *      Author: belyaev
 */

#ifndef PRINTSTATSPASS_H_
#define PRINTSTATSPASS_H_

#include <llvm/Pass.h>

#include "Logging/logger.hpp"
#include "Util/macros.h"

namespace borealis {

// TODO: ImmutablePass?
class PrintStatsPass : public llvm::ModulePass,
    public borealis::logging::ClassLevelLogging<PrintStatsPass>{
public:

    static constexpr auto loggerDomain() QUICK_RETURN("statistics");

    static char ID;

    virtual bool runOnModule(llvm::Module& M) override;

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        AU.setPreservesAll();
    }

    PrintStatsPass(): llvm::ModulePass(ID) {}
    ~PrintStatsPass() {}
};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* PRINTSTATSPASS_H_ */
