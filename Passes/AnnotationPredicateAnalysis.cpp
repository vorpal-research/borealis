/*
 * AnnotationPredicateAnalysis.cpp
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#include <llvm/Support/InstVisitor.h>

#include "Annotation/Annotation.h"
#include "Annotation/AssertAnnotation.h"
#include "Annotation/LogicAnnotation.h"
#include "Annotation/RequiresAnnotation.h"
#include "Codegen/intrinsics_manager.h"
#include "Passes/AnnotationPredicateAnalysis.h"
#include "Passes/AnnotationProcessor.h"
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
            if (llvm::isa<RequiresAnnotation>(anno) ||
                    llvm::isa<AssertAnnotation>(anno)) {
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
    using namespace::llvm;

    AU.setPreservesAll();

    AUX<AnnotationProcessor>::addRequiredTransitive(AU);
    AUX<MetaInfoTrackerPass>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool AnnotationPredicateAnalysis::runOnFunction(llvm::Function& F) {
    using namespace::llvm;

    TRACE_FUNC;

    init();

    MI = &GetAnalysis<MetaInfoTrackerPass>::doit(this, F);

    SlotTracker* ST = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);

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
