/*
 * AnnotationPredicateAnalysis.cpp
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#include <llvm/Support/InstVisitor.h>

#include "Annotation/Annotation.def"

#include "Codegen/intrinsics_manager.h"
#include "Passes/AnnotationPredicateAnalysis.h"
#include "Passes/SlotTrackerPass.h"
#include "State/AnnotationMaterializer.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////
//
// Annotation predicate extraction visitor
//
////////////////////////////////////////////////////////////////////////////////

class APAInstVisitor : public llvm::InstVisitor<APAInstVisitor> {

public:

    APAInstVisitor(AnnotationPredicateAnalysis* pass) : pass(pass) {}

    void visitCallInst(llvm::CallInst& CI) {
        auto& im = IntrinsicsManager::getInstance();
        if (im.getIntrinsicType(CI) == function_type::INTRINSIC_ANNOTATION) {
            Annotation::Ptr anno =
                    materialize(Annotation::fromIntrinsic(CI), pass->TF.get(), pass->MI);
            if (llvm::isa<AssumeAnnotation>(anno)) {
                LogicAnnotation* LA = llvm::cast<LogicAnnotation>(anno);
                pass->PM[&CI] =
                        pass->PF->getEqualityPredicate(
                                LA->getTerm(),
                                pass->TF->getTrueTerm(),
                                predicateType(LA)
                        );
            }
        }
    }

private:

    AnnotationPredicateAnalysis* pass;

};

////////////////////////////////////////////////////////////////////////////////

AnnotationPredicateAnalysis::AnnotationPredicateAnalysis() :
        ProxyFunctionPass(ID) {}

AnnotationPredicateAnalysis::AnnotationPredicateAnalysis(llvm::Pass* pass) :
        ProxyFunctionPass(ID, pass) {}

void AnnotationPredicateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<MetaInfoTrackerPass>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool AnnotationPredicateAnalysis::runOnFunction(llvm::Function& F) {
    init();

    MI = &GetAnalysis<MetaInfoTrackerPass>::doit(this, F);

    auto* ST = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    PF = PredicateFactory::get(ST);
    TF = TermFactory::get(ST);

    APAInstVisitor visitor(this);
    visitor.visit(F);

    return false;
}

AnnotationPredicateAnalysis::~AnnotationPredicateAnalysis() {}

////////////////////////////////////////////////////////////////////////////////

char AnnotationPredicateAnalysis::ID;
static RegisterPass<AnnotationPredicateAnalysis>
X("annotation-predicate-analysis", "Annotation predicate extraction");

} /* namespace borealis */
