/*
 * DefectManager.cpp
 *
 *  Created on: Jan 24, 2013
 *      Author: ice-phoenix
 */

#include "Passes/Defect/DefectManager.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Util/passes.hpp"

namespace borealis {

DefectManager::DefectManager() : llvm::ModulePass(ID) {}

void DefectManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
}

void DefectManager::addDefect(DefectType type, llvm::Instruction* where) {
    addDefect(getDefect(type, where));
}

void DefectManager::addDefect(const std::string& type, llvm::Instruction* where) {
    addDefect(getDefect(type, where));
}

void DefectManager::addDefect(const DefectInfo& info) {
    data.trueData.insert(info);
    supplemental.insert({info, {}});
}

void DefectManager::addNoDefect(const DefectInfo& info) {
    data.falseData.insert(info);
}

const AdditionalDefectInfo& DefectManager::getAdditionalInfo(const DefectInfo& di) const {
    return supplemental.at(di);
}

AdditionalDefectInfo& DefectManager::getAdditionalInfo(const DefectInfo& di) {
    return supplemental.at(di);
}

DefectInfo DefectManager::getDefect(DefectType type, llvm::Instruction* where) const {
    return getDefect(DefectTypes.at(type).type, where);
}

DefectInfo DefectManager::getDefect(const std::string& type, llvm::Instruction* where) const {
    auto* locs = &GetAnalysis<SourceLocationTracker>::doit(this);
    return {type, locs->getLocFor(where)};
}

bool DefectManager::hasDefect(DefectType type, llvm::Instruction* where) const {
    return hasDefect(DefectTypes.at(type).type, where);
}

bool DefectManager::hasDefect(const std::string& type, llvm::Instruction* where) const {
    return hasDefect(getDefect(type, where));
}

bool DefectManager::hasDefect(const DefectInfo& di) const {
    return util::contains(data.trueData, di) || util::contains(data.truePastData, di);
}

bool DefectManager::hasInfo(const DefectInfo& di) const {
    return util::contains(data.trueData, di) || util::contains(data.truePastData, di) || util::contains(data.falsePastData, di);
}


void DefectManager::print(llvm::raw_ostream&, const llvm::Module*) const {
    for (const auto& defect : data.trueData) {
        infos() << defect.type << " at " << defect.location << endl;
    }
}

char DefectManager::ID;
static RegisterPass<DefectManager>
X("defect-manager", "Pass that collects and filters detected defects");

DefectManager::AdditionalDefectData DefectManager::supplemental;
impl_::persistentDefectData DefectManager::data("persistentDefectData.json");

} /* namespace borealis */
