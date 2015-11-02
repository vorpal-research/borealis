/*
 * AnnotatorPass.cpp
 *
 *  Created on: Oct 8, 2012
 *      Author: belyaev
 */

#include "Annotation/AnnotationCast.h"
#include "Factory/Nest.h"
#include "Passes/Manager/AnnotationManager.h"
#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

void AnnotationManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX< comments >::addRequiredTransitive(AU);
    AUX< slots >::addRequiredTransitive(AU);
}

bool AnnotationManager::runOnModule(llvm::Module& M) {
    using borealis::util::view;

    auto& cmnts = GetAnalysis< comments >::doit(this);

    auto* st = GetAnalysis< slots >::doit(this).getSlotTracker(M);
    auto fn = FactoryNest(M.getDataLayout(), st);

    annotations = cmnts.provide().data();

    return false;
}

void AnnotationManager::print(llvm::raw_ostream&, const llvm::Module*) const {
    for (const auto& An : annotations) {
        infos() << An << endl;
    }
}

char AnnotationManager::ID;
static RegisterPass<AnnotationManager>
X("annotation-manager", "Annotation manager", false, false);

} // namespace borealis
