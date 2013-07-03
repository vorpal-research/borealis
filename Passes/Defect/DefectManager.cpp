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
    addDefect(DefectTypes.at(type).type, where);
}

void DefectManager::addDefect(std::string type, llvm::Instruction* where) {
    auto* locs = &GetAnalysis<SourceLocationTracker>::doit(this);
    data.insert({type, locs->getLocFor(where)});
}

void DefectManager::print(llvm::raw_ostream& O, const llvm::Module*) const {
    for (const auto& defect : data) {
        O << defect.type << " at " << defect.location << util::streams::endl;
    }
}

char DefectManager::ID;
static RegisterPass<DefectManager>
X("defect-manager", "Pass that collects and filters detected defects");

DefectManager::DefectData DefectManager::data;

} /* namespace borealis */
