/*
 * DefectManager.h
 *
 *  Created on: Jan 24, 2013
 *      Author: ice-phoenix
 */

#ifndef DEFECTMANAGER_H_
#define DEFECTMANAGER_H_

#include <llvm/Pass.h>

#include <set>

#include "Logging/logger.hpp"
#include "Passes/DefectManager/DefectInfo.h"

namespace borealis {

class DefectManager :
        public llvm::ModulePass,
        public borealis::logging::ClassLevelLogging<DefectManager> {

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("defect-manager")
#include "Util/unmacros.h"

    typedef std::set<DefectInfo> DefectData;
    typedef DefectData::value_type DefectDataEntry;

    static char ID;

    DefectManager();
    virtual bool runOnModule(llvm::Module&) override { return false; }
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~DefectManager() {};

    void addDefect(DefectType type, llvm::Instruction* where);

    virtual void print(llvm::raw_ostream&, const llvm::Module*) const override;

private:

    static DefectData data;

public:

    const DefectData& getData() const { return data; }

#include "Util/macros.h"
    auto begin() QUICK_CONST_RETURN(data.begin())
    auto end() QUICK_CONST_RETURN(data.end())
#include "Util/unmacros.h"

};

} /* namespace borealis */

#endif /* DEFECTMANAGER_H_ */
