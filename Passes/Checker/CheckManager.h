/*
 * CheckManager.h
 *
 *  Created on: Dec 9, 2013
 *      Author: ice-phoenix
 */

#ifndef CHECKMANAGER_H_
#define CHECKMANAGER_H_

#include <llvm/Pass.h>

#include <set>

#include "Logging/logger.hpp"
#include "Util/util.h"

namespace borealis {

class CheckManager :
        public llvm::ImmutablePass,
        public borealis::logging::ClassLevelLogging<CheckManager> {

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("check-manager")
#include "Util/unmacros.h"

    CheckManager();
    virtual void initializePass() override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~CheckManager();

    bool shouldSkipFunction(llvm::Function* F) const;

private:

    std::unordered_set<std::string> includes;
    std::unordered_set<std::string> excludes;

};

} /* namespace borealis */

#endif /* CHECKMANAGER_H_ */
