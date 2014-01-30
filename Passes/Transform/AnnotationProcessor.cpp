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
#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

void AnnotationProcessor::getAnalysisUsage(llvm::AnalysisUsage& AU) const{
    AU.setPreservesCFG();

    AUX<AnnotationManager>::addRequired(AU);
    AUX<SourceLocationTracker>::addRequired(AU);
}

bool AnnotationProcessor::runOnModule(llvm::Module& M) {
    using namespace llvm;
    using llvm::Type;
    using borealis::util::reverse;
    using borealis::util::toString;
    using borealis::util::view;

    auto& im = IntrinsicsManager::getInstance();

    auto& annotations = GetAnalysis< AnnotationManager >::doit(this);
    auto& locs = GetAnalysis< SourceLocationTracker >::doit(this);

    for (auto anno : annotations) {
        if (!isa<LogicAnnotation>(anno)) continue;

        Constant* data = ConstantDataArray::getString(M.getContext(), toString(anno));

        Function* anno_intr = im.createIntrinsic(
                function_type::INTRINSIC_ANNOTATION,
                toString(*data->getType()),
                FunctionType::get(
                        Type::getVoidTy(M.getContext()),
                        data->getType(),
                        false),
                &M
        );

        CallInst* tmpl = CallInst::Create(
                anno_intr,
                data,
                "");
        tmpl->setMetadata("anno.ptr", ptr2MDNode(M.getContext(), anno.get()));

        if (isa<RequiresAnnotation>(anno) ||
            isa<AssignsAnnotation>(anno)) {
            for (auto& e : view(locs.getRangeFor(anno->getLocus()))) {
                if (Function* F = dyn_cast<Function>(e.second)) {
                    // FIXME akhin Fix annotation location business in SourceLocationTracker / MetaInfoTracker
                    // insertBeforeWithLocus(tmpl, F->getEntryBlock().getFirstNonPHIOrDbgOrLifetime(), anno->getLocus());
                    tmpl->insertBefore(F->getEntryBlock().getFirstNonPHIOrDbgOrLifetime());
                    break;
                }
            }

        } else if (isa<EnsuresAnnotation>(anno)) {
            for (auto& e : view(locs.getRangeFor(anno->getLocus()))) {
                if (Function* F = dyn_cast<Function>(e.second)) {
                    // this is generally fucked up, BUT we have to insert the template somewhere
                    // otherwise it cannot be properly deleted
                    //
                    // FIXME akhin Fix annotation location business in SourceLocationTracker / MetaInfoTracker
                    // insertBeforeWithLocus(tmpl, F->getEntryBlock().getFirstNonPHIOrDbgOrLifetime(), anno->getLocus());
                    tmpl->insertBefore(F->getEntryBlock().getFirstNonPHIOrDbgOrLifetime());
                    for (auto& BB : *F) {
                        if (isa<ReturnInst>(BB.getTerminator()) || isa<UnreachableInst>(BB.getTerminator())) {
                            // insert clones at the actual rets
                            // no need to clone MDNodes, they are copied in templ->clone()
                            //
                            // FIXME akhin Fix annotation location business in SourceLocationTracker / MetaInfoTracker
                            // insertBeforeWithLocus(tmpl->clone(), BB.getTerminator(), anno->getLocus());
                            tmpl->clone()->insertBefore(BB.getTerminator());
                        }
                    }
                    tmpl->eraseFromParent();
                    break;
                }
            }

        } else if (isa<AssertAnnotation>(anno) || isa<AssumeAnnotation>(anno)) {
            for (auto& e : view(locs.getRangeFor(anno->getLocus()))) {
                if (Instruction* I = dyn_cast<Instruction>(e.second)) {
                    // FIXME akhin Fix annotation location business in SourceLocationTracker / MetaInfoTracker
                    insertBeforeWithLocus(tmpl, I, anno->getLocus());
                    // tmpl->insertBefore(I);
                    break;
                }
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
