/*
 * FunctionManager.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#include "Annotation/LogicAnnotation.h"
#include "Codegen/intrinsics.h"
#include "Codegen/intrinsics_manager.h"
#include "Passes/AnnotatorPass.h"
#include "Passes/FunctionManager.h"
#include "Passes/MetaInfoTrackerPass.h"
#include "Passes/SourceLocationTracker.h"
#include "Predicate/PredicateFactory.h"
#include "State/AnnotationMaterializer.h"
#include "Term/TermFactory.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

using borealis::util::view;

FunctionManager::FunctionManager() : llvm::ModulePass(ID) {}

void FunctionManager::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<AnnotatorPass>::addRequiredTransitive(AU);
    AUX<MetaInfoTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
}

bool FunctionManager::runOnModule(llvm::Module& /* M */) {
    using namespace llvm;

    AnnotatorPass& annotations = GetAnalysis< AnnotatorPass >::doit(this);
    MetaInfoTrackerPass& meta =  GetAnalysis< MetaInfoTrackerPass >::doit(this);
    SourceLocationTracker& locs = GetAnalysis< SourceLocationTracker >::doit(this);

    PredicateFactory::Ptr PF = PredicateFactory::get(nullptr); // FIXME: Add SlotTrackerPass
    TermFactory::Ptr TF = TermFactory::get(nullptr); // FIXME: Add SlotTrackerPass

    for (Annotation::Ptr a : annotations) {
        Annotation::Ptr anno = materialize(a, TF.get(), &meta);
        if (auto* logic = dyn_cast<LogicAnnotation>(anno)) {
            for (auto& e : view(locs.getRangeFor(logic->getLocus()))) {
                if (Function* F = dyn_cast<Function>(e.second)) {
                    Predicate::Ptr p = PF->getEqualityPredicate(
                            logic->getTerm(),
                            TF->getTrueTerm(),
                            predicateType(logic)
                    );
                    update(F, p);
                    break;
                }
            }
        }
    }

    return false;
}

void FunctionManager::put(llvm::Function* F, const PredicateState& state) {

    using borealis::util::containsKey;

    ASSERT(!containsKey(data, F),
           "Attempt to register function " + F->getName().str() + " twice")

    data[F] = state;
}

void FunctionManager::update(llvm::Function* F, const PredicateState& state) {
    data[F] = data[F] + state;
}

const PredicateState& FunctionManager::get(
        llvm::CallInst& CI,
        PredicateFactory* PF,
        TermFactory* TF) {

    using borealis::util::containsKey;

    llvm::Function* F = CI.getCalledFunction();

    if (!containsKey(data, F)) {
        auto& m = IntrinsicsManager::getInstance();

        function_type ft = m.getIntrinsicType(CI);
        if (!isUnknown(ft)) {
            auto state = m.getPredicateState(ft, F, PF, TF);
            data[F] = state;
            return data.at(F);
        } else {
            return PredicateState::empty();
        }
    } else {
        return data.at(F);
    }

    BYE_BYE(PredicateState&, "Unreachable!");
}

char FunctionManager::ID;
static RegisterPass<FunctionManager>
X("function-manager", "Pass that manages function analysis results");

} /* namespace borealis */

#include "Util/unmacros.h"
