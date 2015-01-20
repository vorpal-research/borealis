/*
 * AnnotationPredicateAnalysis.cpp
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#include <llvm/IR/InstVisitor.h>

#include "Annotation/Annotation.def"
#include "Codegen/intrinsics_manager.h"
#include "Passes/PredicateAnalysis/AnnotationPredicateAnalysis.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "State/Transformer/AnnotationMaterializer.h"

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
                    materialize(Annotation::fromIntrinsic(CI), pass->FN, pass->MI);
            if (llvm::isa<AssumeAnnotation>(anno)) {
                const LogicAnnotation* LA = llvm::cast<LogicAnnotation>(anno);
                pass->PM[&CI] =
                    pass->FN.Predicate->getEqualityPredicate(
                        LA->getTerm(),
                        pass->FN.Term->getTrueTerm(),
                        pass->SLT->getLocFor(&CI),
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

    AUX<MetaInfoTracker>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
}

bool AnnotationPredicateAnalysis::runOnFunction(llvm::Function& F) {
    init();

    MI = &GetAnalysis<MetaInfoTracker>::doit(this, F);
    SLT = &GetAnalysis<SourceLocationTracker>::doit(this, F);

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(st);

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
