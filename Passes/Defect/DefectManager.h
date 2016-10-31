/*
 * DefectManager.h
 *
 *  Created on: Jan 24, 2013
 *      Author: ice-phoenix
 */

#ifndef DEFECTMANAGER_H_
#define DEFECTMANAGER_H_

#include <llvm/Pass.h>
#include <llvm/Support/LockFileManager.h>

#include <set>
#include <fstream>
#include <Config/config.h>

#include "Logging/logger.hpp"
#include "Passes/Defect/DefectManager/DefectInfo.h"
#include "Util/json.hpp"

namespace borealis {

struct AdditionalDefectInfo {
    llvm::Function* atFunc;
    llvm::Instruction* atInst;
    util::option<smt::SatResult> satModel;

    enum class RunResult { Proven, Disproven, Controversal, NotRun };
    RunResult runResult = RunResult::NotRun;
};

namespace impl_ {

static config::BoolConfigEntry usePersistentDefectData("analysis", "persistent-defect-data");

struct persistentDefectData {
    using DefectData = std::unordered_set<DefectInfo>;
    DefectData trueData;
    DefectData falseData;
    DefectData truePastData;
    DefectData falsePastData;

    using SimpleT = std::pair< DefectData, DefectData >;

    std::string filename;

    template<class Body>
    void locked(Body body) {
        while(true) {
            llvm::LockFileManager fileLock(filename);
            if(fileLock == llvm::LockFileManager::LFS_Shared) {
                fileLock.waitForUnlock();
                continue;
            }
            if(fileLock == llvm::LockFileManager::LFS_Error) {
                errs() << "error while trying to lock file \"" << filename << "\"" << endl;
            }

            body();
            break;
        }
    }

    persistentDefectData(const std::string& filename): trueData(), falseData(), filename(filename) {
        if(usePersistentDefectData.get(false)) {
            locked([&](){
                std::ifstream in(filename);
                if(auto&& loaded = util::read_as_json<SimpleT>(in)) {
                    truePastData = std::move(loaded->first);
                    falsePastData = std::move(loaded->second);
                }
            });
        }
    }

    persistentDefectData(const char* filename): persistentDefectData(std::string(filename)) {}

    ~persistentDefectData() {
//        if(usePersistentDefectData.get(false)) {
//            for (auto&& e : trueData) falseData.erase(e);
//            truePastData.insert(trueData.begin(), trueData.end());
//            falsePastData.insert(falseData.begin(), falseData.end());
//
//            std::ofstream out{filename};
//            util::write_as_json(out, std::make_pair(truePastData, falsePastData));
//        }
    }

    void forceDump() {
        if(usePersistentDefectData.get(false)) {
            auto tpd = truePastData;
            auto fpd = falsePastData;
            for (auto&& e : trueData) falseData.erase(e);
            tpd.insert(trueData.begin(), trueData.end());
            fpd.insert(falseData.begin(), falseData.end());

            locked([&](){
                {
                    std::ifstream in{filename};
                    if(auto existing = util::read_as_json<SimpleT>(in)) {
                        tpd.insert(std::make_move_iterator(existing->first.begin()), std::make_move_iterator(existing->first.end()));
                        fpd.insert(std::make_move_iterator(existing->second.begin()), std::make_move_iterator(existing->second.end()));
                    }
                }

                std::ofstream out{filename};
                util::write_as_json(out, std::make_pair(std::move(tpd), std::move(fpd)));
             });

        }
    }
};

} /* namespace impl_ */

class
DefectManager :
        public llvm::ModulePass,
        public borealis::logging::ClassLevelLogging<DefectManager> {

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("defect-manager")
#include "Util/unmacros.h"

    typedef std::unordered_set<DefectInfo> DefectData;
    typedef std::unordered_map<DefectInfo, AdditionalDefectInfo> AdditionalDefectData;

    static char ID;

    DefectManager();
    virtual bool runOnModule(llvm::Module&) override { return false; }

    virtual bool doFinalization(llvm::Module &module) override;

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~DefectManager() {
        // this is a bit fucked up
        getStaticData().forceDump();
    };

    void addDefect(DefectType type, llvm::Instruction* where);
    void addDefect(const std::string& type, llvm::Instruction* where);
    void addDefect(const DefectInfo& info);

    void addNoDefect(const DefectInfo& info);

    const AdditionalDefectInfo& getAdditionalInfo(const DefectInfo&) const;
    AdditionalDefectInfo& getAdditionalInfo(const DefectInfo&);

    DefectInfo getDefect(DefectType type, llvm::Instruction* where) const;
    DefectInfo getDefect(const std::string& type, llvm::Instruction* where) const;

    bool hasDefect(DefectType type, llvm::Instruction* where) const;
    bool hasDefect(const std::string& type, llvm::Instruction* where) const;
    bool hasDefect(const DefectInfo& di) const;

    bool hasInfo(const DefectInfo& di) const;

    virtual void print(llvm::raw_ostream&, const llvm::Module*) const override;

private:

    static impl_::persistentDefectData& getStaticData() {
        static impl_::persistentDefectData data("persistentDefectData.json");
        return data;
    }

    static AdditionalDefectData& getSupplemental() {
        static AdditionalDefectData data;
        return data;
    }

public:

    const DefectData& getData() const { return getStaticData().trueData; }

#include "Util/macros.h"
    auto begin() QUICK_CONST_RETURN(getStaticData().trueData.begin())
    auto end() QUICK_CONST_RETURN(getStaticData().trueData.end())
#include "Util/unmacros.h"

};

} /* namespace borealis */

#endif /* DEFECTMANAGER_H_ */
