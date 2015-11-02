/*
 * AnnotationProcessor.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: belyaev
 */

#include <llvm/IR/Constants.h>



#include "Codegen/intrinsics_manager.h"
#include "Codegen/llvm.h"
#include "Passes/Manager/AnnotationManager.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Passes/Transform/AnnotationProcessor.h"
#include "State/Transformer/AnnotationSubstitutor.h"
#include "State/Transformer/AnnotationMaterializer.h"
#include "State/Transformer/TermCollector.h"
#include "State/Transformer/TermValueMapper.h"
#include "State/Transformer/OldValueExtractor.h"
#include "Util/passes.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

void AnnotationProcessor::getAnalysisUsage(llvm::AnalysisUsage& AU) const{
    AU.setPreservesCFG();

    AUX<AnnotationManager>::addRequired(AU);
    AUX<SourceLocationTracker>::addRequired(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<VariableInfoTracker>::addRequiredTransitive(AU);
}

static llvm::CallInst* getAnnotationCall(llvm::Module& M,
                                         FactoryNest FN,
                                         Annotation::Ptr anno,
                                         const std::set<Term::Ptr, TermCompare>& supplemental,
                                         const llvm::Instruction* hint) {
    using namespace llvm;
    using namespace borealis::util;
    using llvm::Type; // clash with borealis::Type

    TermValueMapper tvm(FN, hint);
    for(auto&& term : supplemental) tvm.transform(term);


    auto& im = IntrinsicsManager::getInstance();
    Constant* data = ConstantDataArray::getString(M.getContext(), toString(anno));
    auto strType = data->getType();

    auto callArgs = (util::viewSingleReference(data) >> util::viewContainer(supplemental).map(APPLY(tvm.getMapping().at)))
                   .map(LAM(it, const_cast<llvm::Value*>(it)))
                   .toVector();

    Function* anno_intr = im.createIntrinsic(
        function_type::INTRINSIC_ANNOTATION,
        toString(*strType),
        FunctionType::get(
            Type::getVoidTy(M.getContext()),
            strType,
            true /* isVarArg */
        ),
        &M
    );

    CallInst* tmpl = CallInst::Create(anno_intr, callArgs, "");
    Annotation::installOnIntrinsic(*tmpl, anno);
    return tmpl;
}

bool AnnotationProcessor::landOnInstructionOrFirst(Annotation::Ptr anno,
                                     llvm::Module& M,
                                     FactoryNest FN,
                                     llvm::Value& val) {

    auto supplemental = substitutionOrdering(anno);

    if(auto F = llvm::dyn_cast<llvm::Function>(&val)) {
        auto hint = F->getEntryBlock().getFirstNonPHIOrDbgOrLifetime();
        auto template_ = getAnnotationCall(M, FN, anno, supplemental, hint);
        // FIXME akhin Fix annotation location business in SourceLocationTracker / MetaInfoTracker
        template_->insertBefore(hint);
        return true;
    } else if (auto I = llvm::dyn_cast<llvm::Instruction>(&val)) {
        auto template_ = getAnnotationCall(M, FN, anno, supplemental, I);
        insertBeforeWithLocus(template_, I, anno->getLocus());
        return true;
    }
    return false;
}

bool AnnotationProcessor::landOnInstructionOrLast(Annotation::Ptr anno, llvm::Module& M, FactoryNest FN, llvm::Value& val) {
    using namespace llvm;

    auto supplemental = substitutionOrdering(anno);

    if(auto F = llvm::dyn_cast<llvm::Function>(&val)) {
        // FIXME akhin Fix annotation location business in SourceLocationTracker / MetaInfoTracker
        for(auto& BB: *F) {
            auto Term = BB.getTerminator();
            if (isa<ReturnInst>(Term) || isa<UnreachableInst>(Term)) {
                // insert clones at the actual rets
                // no need to clone MDNodes, they are copied in templ->clone()
                //
                // FIXME akhin Fix annotation location business in SourceLocationTracker / MetaInfoTracker
                // insertBeforeWithLocus(tmpl->clone(), BB.getTerminator(), anno->getLocus());
                auto template_ = getAnnotationCall(M, FN, anno, supplemental, Term);
                template_->insertBefore(Term);
            }
        }

        return true;
    } else if (auto I = llvm::dyn_cast<llvm::Instruction>(&val)) {
        auto template_ = getAnnotationCall(M, FN, anno, supplemental, I);
        insertAfterWithLocus(template_, I, anno->getLocus());
        return true;
    }
    return false;
}

static llvm::Function* getEnclosingFunction(llvm::Value& v) {
    if(auto F = llvm::dyn_cast<llvm::Function>(&v)) return F;
    if(auto I = llvm::dyn_cast<llvm::Instruction>(&v)) return I->getParent()->getParent();
    return nullptr;
}

// FIXME: this is generally fucked up
static Term::Ptr transformIgnoringErrors(AnnotationMaterializer& AM, Term::Ptr term) {
    try {
        return AM.transform(term);
    } catch(...) {
        return nullptr;
    }
}

bool AnnotationProcessor::runOnModule(llvm::Module& M) {
    using namespace llvm;
    using llvm::Type;
    using borealis::util::reverse;
    using borealis::util::toString;
    using borealis::util::view;

    auto& annotations = GetAnalysis< AnnotationManager >::doit(this);
    auto& locs = GetAnalysis< SourceLocationTracker >::doit(this);


    auto VIT = &GetAnalysis< VariableInfoTracker > ::doit(this);
    auto& IM = IntrinsicsManager::getInstance();

    for (auto anno : annotations) {
        if (!isa<LogicAnnotation>(anno)) continue;

        if(isa<GlobalAnnotation>(anno)) {
            for(auto&& F: M) if(!F.isDeclaration() && IM.getIntrinsicType(&F) == function_type::UNKNOWN){
                auto st = GetAnalysis< SlotTrackerPass >::doit(this).getSlotTracker(F);
                auto FN = FactoryNest{ F.getDataLayout(), st };
                try{
                    anno = materialize(anno, FN, VIT);
                    landOnInstructionOrFirst(anno, M, FN, F);
                } catch(std::exception& ex) {
                    infos() << "Unable to materialize annotation: " << *anno << ":" << ex.what() << endl;
                }
            }
            continue;
        }

        auto possibleLocsView = view(locs.getRangeFor(anno->getLocus()));
        llvm::Function* context = nullptr;
        llvm::Value* boundValue = nullptr;

        for(auto&& e : possibleLocsView) {
            if(llvm::is_one_of<llvm::Instruction, llvm::Function>(*e.second)) {
                boundValue = e.second;
                context = getEnclosingFunction(*boundValue);
                break;
            }
        }

        auto st = GetAnalysis< SlotTrackerPass >::doit(this).getSlotTracker(context);
        auto FN = FactoryNest{ M.getDataLayout(), st };

        OldValueExtractor ove{FN};

        anno = ove.transform(anno);
        anno = materialize(anno, FN, VIT);

        // ensures are placed at the end of the func
        auto handler =
            isa<EnsuresAnnotation>(anno) ?
                &landOnInstructionOrLast :
                &landOnInstructionOrFirst;

        handler(anno, M, FN, *boundValue);

        // context-bound synthetic assumes are pushed to the beginning of enclosing function
        if(context) {
            for(auto anno : ove.getResults()) {
                anno = materialize(anno, FN, VIT);
                landOnInstructionOrFirst(anno, M, FN, *context);
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

#include "Util/unmacros.h"
