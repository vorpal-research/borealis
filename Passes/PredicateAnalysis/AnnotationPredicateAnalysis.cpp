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
#include "State/Transformer/AnnotationSubstitutor.h"
#include "Util/cache.hpp"

#include "Util/macros.h"

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
                    substituteAnnotationCall(pass->FN, llvm::CallSite(&CI));
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

            if (llvm::isa<AssignsAnnotation>(anno)) {
                const LogicAnnotation* LA = llvm::cast<LogicAnnotation>(anno);
                auto trm = LA->getTerm();
                static auto freshVarCache = util::make_size_t_cache<const llvm::Instruction*>();
                auto currentSeed = freshVarCache.at(&CI);

                if (llvm::isa<ReadPropertyTerm>(trm)) {
                    auto rpt = llvm::cast<ReadPropertyTerm>(trm);
                    pass->PM[&CI] =
                        pass->FN.Predicate->getWritePropertyPredicate(
                            rpt->getPropertyName(),
                            rpt->getRhv(),
                            pass->FN.Term->getFreeVarTerm(pass->FN.Type->getInteger(), "borealis.fresh.var." + util::toString(currentSeed)),
                            pass->SLT->getLocFor(&CI)
                        );
                } else if (llvm::isa<BoundTerm>(trm)) {
                    auto rpt = llvm::cast<BoundTerm>(trm);
                    pass->PM[&CI] =
                        pass->FN.Predicate->getWriteBoundPredicate(
                            rpt->getRhv(),
                            pass->FN.Term->getFreeVarTerm(pass->FN.Type->getInteger(), "borealis.fresh.var." + util::toString(currentSeed)),
                            pass->SLT->getLocFor(&CI)
                        );
                } else if (llvm::isa<LoadTerm>(trm)) {
                    auto rpt = llvm::cast<LoadTerm>(trm);
                    ASSERTC(llvm::isa<type::Pointer>(rpt->getType()));
                    auto ptr = llvm::cast<type::Pointer>(rpt->getType());
                    pass->PM[&CI] =
                        pass->FN.Predicate->getStorePredicate(
                            rpt->getRhv(),
                            pass->FN.Term->getFreeVarTerm(ptr->getPointed(), "borealis.fresh.var." + util::toString(currentSeed)),
                            pass->SLT->getLocFor(&CI)
                        );
                }
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

    AUX<VariableInfoTracker>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
}

bool AnnotationPredicateAnalysis::runOnFunction(llvm::Function& F) {
    init();

    MI = &GetAnalysis<VariableInfoTracker>::doit(this, F);
    SLT = &GetAnalysis<SourceLocationTracker>::doit(this, F);

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(F.getDataLayout(), st);

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

#include "Util/unmacros.h"
