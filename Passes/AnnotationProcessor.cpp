/*
 * AnnotationProcessor.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: belyaev
 */

#include <llvm/Constants.h>

#include "Codegen/intrinsics_manager.h"
#include "Passes/AnnotationProcessor.h"
#include "Passes/AnnotatorPass.h"
#include "Passes/SourceLocationTracker.h"
#include "State/AnnotationMaterializer.h"
#include "Util/util.h"

namespace borealis {

void AnnotationProcessor::getAnalysisUsage(llvm::AnalysisUsage& AU) const{
    AU.setPreservesAll();
    AUX<AnnotatorPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
}

static llvm::MDNode* ptr2MDNode(llvm::LLVMContext& ctx, void* ptr) {
    llvm::Constant* annoptr = llvm::ConstantInt::get(
            ctx,
            llvm::APInt(sizeof(uintptr_t)*8, reinterpret_cast<uintptr_t>(ptr))
    );
   return llvm::MDNode::get(ctx, annoptr);
}

static void* MDNode2Ptr(llvm::MDNode* ptr) {
    if(auto* i = llvm::dyn_cast<llvm::ConstantInt>(ptr->getOperand(0))) {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(i->getLimitedValue()));
    }
    return nullptr;
}

bool AnnotationProcessor::runOnModule(llvm::Module& M) {
    using namespace llvm;
    using llvm::Type;
    using borealis::util::reverse;
    using borealis::util::toString;
    using borealis::util::view;

    IntrinsicsManager& im = IntrinsicsManager::getInstance();

    AnnotatorPass& annotations = GetAnalysis< AnnotatorPass >::doit(this);
    SourceLocationTracker& locs = GetAnalysis< SourceLocationTracker >::doit(this);

    for (Annotation::Ptr anno : reverse(annotations)) {

        if(!isa<LogicAnnotation>(anno)) continue;

        Constant* data = ConstantDataArray::getString(M.getContext(), toString(*anno));

        Function* anno_intr = im.createIntrinsic(
                function_type::INTRINSIC_ANNOTATION,
                toString(*data->getType()),
                FunctionType::get(
                        Type::getVoidTy(M.getContext()),
                        data->getType(),
                        false),
                &M
        );

        if (isa<RequiresAnnotation>(anno) ||
                isa<EnsuresAnnotation>(anno) ||
                isa<AssignsAnnotation>(anno)) {
            for (auto& e : view(locs.getRangeFor(anno->getLocus()))) {
                if (Function* F = dyn_cast<Function>(e.second)) {
                    CallInst::Create(
                            anno_intr,
                            data,
                            "",
                            F->getEntryBlock().getFirstNonPHIOrDbgOrLifetime())->
                                setMetadata("anno.ptr", ptr2MDNode(M.getContext(), anno.get()));
                    break;
                }
            }
        } else if (isa<AssertAnnotation>(anno)) {
            for (auto& e : view(locs.getRangeFor(anno->getLocus()))) {
                if (Instruction* I = dyn_cast<Instruction>(e.second)) {
                    CallInst::Create(
                            anno_intr,
                            data,
                            "",
                            I)->
                        setMetadata("anno.ptr", ptr2MDNode(M.getContext(), anno.get()));
                    break;
                }
            }
        }
    }

    return false;
}

void AnnotationProcessor::print(llvm::raw_ostream&, const llvm::Module* M) const {
    auto& IM = IntrinsicsManager::getInstance();

    for(auto& F : *M) {
        for(auto& BB : F) {
            for(auto& I : BB) {
                if(auto* ci = llvm::dyn_cast<llvm::CallInst>(&I)) {
                    if(IM.getIntrinsicType(*ci) == function_type::INTRINSIC_ANNOTATION) {
                        infos() << *static_cast<Annotation*>(MDNode2Ptr(ci->getMetadata("anno.ptr"))) << endl;
                    }
                }
            }
        }
    }
}

char AnnotationProcessor::ID;
static RegisterPass<AnnotationProcessor>
X("annotation-processor", "Anno annotation language processor");

} /* namespace borealis */
