/*
 * AnnotationProcessor.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: belyaev
 */

#include <llvm/Constants.h>

#include "Codegen/intrinsics_manager.h"
#include "Codegen/llvm.h"
#include "Passes/Manager/AnnotationManager.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Passes/Transform/AnnotationProcessor.h"
#include "State/Transformer/AnnotationMaterializer.h"
#include "State/Transformer/OldValueExtractor.h"
#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

void AnnotationProcessor::getAnalysisUsage(llvm::AnalysisUsage& AU) const{
    AU.setPreservesCFG();

    AUX<AnnotationManager>::addRequired(AU);
    AUX<SourceLocationTracker>::addRequired(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

static llvm::CallInst* getAnnotationCall(llvm::Module& M, Annotation::Ptr anno) {
    using namespace llvm;
    using namespace borealis::util;
    using llvm::Type; // clash with borealis::Type

    auto& im = IntrinsicsManager::getInstance();
    Constant* data = ConstantDataArray::getString(M.getContext(), toString(anno));
    auto strType = data->getType();

    Function* anno_intr = im.createIntrinsic(
        function_type::INTRINSIC_ANNOTATION,
        toString(*strType),
        FunctionType::get(
            Type::getVoidTy(M.getContext()),
            strType,
            false /* isVarArg */
        ),
        &M
    );

    CallInst* tmpl = CallInst::Create(anno_intr, data, "");
    tmpl->setMetadata("anno.ptr", ptr2MDNode(M.getContext(), anno.get()));
    return tmpl;
}

static bool landOnInstructionOrFirst(Annotation::Ptr anno, llvm::Module& M, llvm::Value& val) {
    auto template_ = getAnnotationCall(M, anno);

    if(auto F = llvm::dyn_cast<llvm::Function>(&val)) {
        // FIXME akhin Fix annotation location business in SourceLocationTracker / MetaInfoTracker
        template_->insertBefore(F->getEntryBlock().getFirstNonPHIOrDbgOrLifetime());
        return true;
    } else if (auto I = llvm::dyn_cast<llvm::Instruction>(&val)) {
        insertBeforeWithLocus(template_, I, anno->getLocus());
        return true;
    }
    return false;
}

static bool landOnInstructionOrLast(Annotation::Ptr anno, llvm::Module& M, llvm::Value& val) {
    using namespace llvm;
    auto template_ = getAnnotationCall(M, anno);

    if(auto F = llvm::dyn_cast<llvm::Function>(&val)) {
        // FIXME akhin Fix annotation location business in SourceLocationTracker / MetaInfoTracker
        auto first = F->getEntryBlock().getFirstNonPHIOrDbgOrLifetime();
        template_->insertBefore(first);

        for(auto& BB: *F) {
            auto Term = BB.getTerminator();
            if (isa<ReturnInst>(Term) || isa<UnreachableInst>(Term)) {
                // insert clones at the actual rets
                // no need to clone MDNodes, they are copied in templ->clone()
                //
                // FIXME akhin Fix annotation location business in SourceLocationTracker / MetaInfoTracker
                // insertBeforeWithLocus(tmpl->clone(), BB.getTerminator(), anno->getLocus());
                template_->clone()->insertBefore(Term);
            }
        }
        template_->eraseFromParent();

        return true;
    } else if (auto I = llvm::dyn_cast<llvm::Instruction>(&val)) {
        insertBeforeWithLocus(template_, I, anno->getLocus());
        return true;
    }
    return false;
}

static llvm::Function* getEnclosingFunction(llvm::Value& v) {
    if(auto F = llvm::dyn_cast<llvm::Function>(&v)) return F;
    if(auto I = llvm::dyn_cast<llvm::Instruction>(&v)) return I->getParent()->getParent();
    return nullptr;
}

bool AnnotationProcessor::runOnModule(llvm::Module& M) {
    using namespace llvm;
    using llvm::Type;
    using borealis::util::reverse;
    using borealis::util::toString;
    using borealis::util::view;

    auto& annotations = GetAnalysis< AnnotationManager >::doit(this);
    auto& locs = GetAnalysis< SourceLocationTracker >::doit(this);

    auto st = GetAnalysis< SlotTrackerPass >::doit(this).getSlotTracker(M);
    auto FN = FactoryNest{ st };

    OldValueExtractor ove{FN};

    for (auto anno : annotations) {
        if (!isa<LogicAnnotation>(anno)) continue;

        anno = ove.transform(anno);

        auto possibleLocsView = view(locs.getRangeFor(anno->getLocus()));

        // ensures are placed at the end of the func
        auto handler =
            isa<EnsuresAnnotation>(anno) ?
                &landOnInstructionOrLast :
                &landOnInstructionOrFirst;

        llvm::Function* context = nullptr;

        for (auto& e : possibleLocsView) {
            context = getEnclosingFunction(*e.second);
            if(handler(anno, M, *e.second)) break;
        }

        // context-bound synthetic assumes are pushed to the beginning of enclosing function
        if(context) {
            for(auto& anno : ove.getResults()) {
                landOnInstructionOrFirst(anno, M, *context);
            }
        }
    }

    return false;
}

void AnnotationProcessor::print(llvm::raw_ostream&, const llvm::Module* M) const {
    using namespace llvm;
    using borealis::util::viewContainer;

    auto& IM = IntrinsicsManager::getInstance();

    for (auto& I : viewContainer(*M).flatten().flatten()) {
        if (auto* CI = dyn_cast<CallInst>(&I)) {
            if (IM.getIntrinsicType(*CI) == function_type::INTRINSIC_ANNOTATION) {
                infos() << Annotation::fromIntrinsic(*CI) << endl;
            }
        }
    }
}

char AnnotationProcessor::ID;
static RegisterPass<AnnotationProcessor>
X("annotation-processor", "Anno annotation language processor");

} /* namespace borealis */
