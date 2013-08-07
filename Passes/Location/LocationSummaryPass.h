/*
 * LocationSummaryPass.h
 *
 *  Created on: Aug 7, 2013
 *      Author: snowball
 */

#ifndef LOCATIONSUMMARYPASS_H_
#define LOCATIONSUMMARYPASS_H_

#include <llvm/Pass.h>

#include "Logging/logger.hpp"


namespace borealis {

class LocationSummaryPass:
        public llvm::ModulePass,
        public logging::ClassLevelLogging<LocationSummaryPass> {
public:
    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("location-summary")
#include "Util/unmacros.h"

    LocationSummaryPass();
    virtual void getAnalysisUsage(llvm::AnalysisUsage&) const override;
    virtual bool runOnModule(llvm::Module&) override;
    virtual ~LocationSummaryPass() = default;
};

} /* namespace borealis */
#endif /* LOCATIONSUMMARYPASS_H_ */
