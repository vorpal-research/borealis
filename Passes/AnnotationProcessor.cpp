/*
 * AnnotationProcessor.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: belyaev
 */

#include <llvm/Constants.h>

#include "Codegen/intrinsics_manager.h"
#include "Codegen/llvm.h"
#include "Passes/AnnotationProcessor.h"
#include "Passes/AnnotatorPass.h"
#include "Passes/SourceLocationTracker.h"
#include "State/AnnotationMaterializer.h"
#include "Util/util.h"
#include "Util/iterators.hpp"

namespace borealis {

void AnnotationProcessor::getAnalysisUsage(llvm::AnalysisUsage& AU) const{
    AU.setPreservesAll();
    AUX<AnnotatorPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
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

    using borealis::util::view;
    using borealis::util::begin_end_pair;

    for (auto& I : view(begin_end_pair(*M)).flatten().flatten()) {
        if (auto* CI = llvm::dyn_cast<llvm::CallInst>(&I)) {
            if (IM.getIntrinsicType(*CI) == function_type::INTRINSIC_ANNOTATION) {
                infos() << *Annotation::fromIntrinsic(*CI) << endl;
            }
        }
    }
}

char AnnotationProcessor::ID;
static RegisterPass<AnnotationProcessor>
X("annotation-processor", "Anno annotation language processor");

} /* namespace borealis */
