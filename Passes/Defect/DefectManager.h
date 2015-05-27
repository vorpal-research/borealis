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
#include "Passes/Defect/DefectManager/DefectInfo.h"

namespace borealis {

struct AdditionalDefectInfo {
    llvm::Function* atFunc;
    llvm::Instruction* atInst;
    util::option<smt::SatResult> satModel;

    enum class RunResult { Proven, Disproven, Controversal, NotRun };
    RunResult runResult = RunResult::NotRun;
};

class
DefectManager :
        public llvm::ModulePass,
        public borealis::logging::ClassLevelLogging<DefectManager> {

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("defect-manager")
#include "Util/unmacros.h"

    typedef std::set<DefectInfo> DefectData;
    typedef std::unordered_map<DefectInfo, AdditionalDefectInfo> AdditionalDefectData;

    static char ID;

    DefectManager();
    virtual bool runOnModule(llvm::Module&) override { return false; }
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~DefectManager() {};

    void addDefect(DefectType type, llvm::Instruction* where);
    void addDefect(const std::string& type, llvm::Instruction* where);
    void addDefect(const DefectInfo& info);

    const AdditionalDefectInfo& getAdditionalInfo(const DefectInfo&) const;
    AdditionalDefectInfo& getAdditionalInfo(const DefectInfo&);

    DefectInfo getDefect(DefectType type, llvm::Instruction* where) const;
    DefectInfo getDefect(const std::string& type, llvm::Instruction* where) const;

    bool hasDefect(DefectType type, llvm::Instruction* where) const;
    bool hasDefect(const std::string& type, llvm::Instruction* where) const;
    bool hasDefect(const DefectInfo& di) const;

    virtual void print(llvm::raw_ostream&, const llvm::Module*) const override;

private:

    static DefectData data;
    static AdditionalDefectData supplemental;

public:

    const DefectData& getData() const { return data; }

#include "Util/macros.h"
    auto begin() QUICK_CONST_RETURN(data.begin())
    auto end() QUICK_CONST_RETURN(data.end())
#include "Util/unmacros.h"

};

} /* namespace borealis */

#endif /* DEFECTMANAGER_H_ */
