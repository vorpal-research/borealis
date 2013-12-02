/*
 * AdaptorPass.h
 *
 *  Created on: Aug 22, 2012
 *      Author: ice-phoenix
 */

#ifndef ADAPTORPASS_H_
#define ADAPTORPASS_H_

#include <llvm/Pass.h>

#include <map>
#include <set>

#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

class AdaptorPass:
        public llvm::ModulePass,
        public borealis::logging::ClassLevelLogging<AdaptorPass> {

public:
    static char ID;

    AdaptorPass();
    virtual bool runOnModule(llvm::Module& M) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~AdaptorPass();
};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* ADAPTORPASS_H_ */
