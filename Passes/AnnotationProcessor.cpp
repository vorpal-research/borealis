/*
 * AnnotationProcessor.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: belyaev
 */

#include "Passes/AnnotationProcessor.h"
#include "Passes/AnnotatorPass.h"
#include "Passes/MetaInfoTrackerPass.h"
#include "Passes/SlotTrackerPass.h"

#include "State/AnnotationMaterializer.h"
#include "Util/util.h"

namespace borealis {

void AnnotationProcessor::getAnalysisUsage(llvm::AnalysisUsage& Info) const{
    Info.setPreservesAll();
    Info.addRequiredTransitive< AnnotatorPass >();
    Info.addRequiredTransitive< MetaInfoTrackerPass >();
    Info.addRequiredTransitive< SlotTrackerPass >();
}

bool AnnotationProcessor::runOnModule(llvm::Module& M) {
    using borealis::util::view;

    auto& annotations = getAnalysis< AnnotatorPass >();
    auto& metas = getAnalysis< MetaInfoTrackerPass >();
    auto& slots = getAnalysis< SlotTrackerPass > ();
    auto TF = TermFactory::get(slots.getSlotTracker(M));

    for(Annotation::Ptr anno: annotations) {
        try{
            auto newAnno = materialize(anno, TF.get(), &metas);
            infos() << *newAnno << endl;
        } catch(const std::runtime_error& e) {
            errs() << e.what() << endl;
        }

    }

    return false;
}

void AnnotationProcessor::print(llvm::raw_ostream&, const llvm::Module*) const {
}

char AnnotationProcessor::ID;
static llvm::RegisterPass<AnnotationProcessor>
X("annotation-processor", "Anno annotation language processor");


} /* namespace borealis */
