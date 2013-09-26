/*
 * MetaInfoTrackerPass.cpp
 *
 *  Created on: Jan 11, 2013
 *      Author: belyaev
 */

#include <llvm/Constants.h>
#include <llvm/Instructions.h>

#include "Codegen/intrinsics_manager.h"
#include "Logging/logger.hpp"
#include "Passes/Tracker/MetaInfoTracker.h"
#include "Util/passes.hpp"

namespace borealis {

MetaInfoTracker::MetaInfoTracker() : ModulePass(ID) {}

MetaInfoTracker::~MetaInfoTracker() {}

void MetaInfoTracker::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<sm_t>::addRequiredTransitive(AU);
}

using namespace llvm;
using borealis::util::option;
using borealis::util::nothing;
using borealis::util::just;

static VarInfo mkVI(const clang::FileManager&, const llvm::DISubprogram& node,
        clang::Decl* ast = nullptr, bool allocated = false) {

    DIType nodeType(node.getType().getTypeArray().getElement(0));

    VarInfo ret{
        just(node.getName().str()),
        just(Locus(node.getFilename().str(), node.getLineNumber(), 0U)),
        allocated ? VarInfo::Allocated : VarInfo::Plain,
        meta2sign(DIType(node.getType().getTypeArray().getElement(0))),
        ast
    };
    return ret;
}

static VarInfo mkVI(const clang::FileManager&, const llvm::DIGlobalVariable& node,
        clang::Decl* ast = nullptr, bool allocated = false) {
    VarInfo ret{
        just(node.getName().str()),
        just(Locus(node.getFilename().str(), node.getLineNumber(), 0U)),
        allocated ? VarInfo::Allocated : VarInfo::Plain,
        meta2sign(node.getType()),
        ast
    };
    return ret;
}

static VarInfo mkVI(const clang::FileManager&, const llvm::DIVariable& node,
        clang::Decl* ast = nullptr, bool allocated = false) {
    VarInfo ret{
        just(node.getName().str()),
        just(Locus(node.getContext().getFilename(), node.getLineNumber(), 0U)),
        allocated ? VarInfo::Allocated : VarInfo::Plain,
        meta2sign(node.getType()),
        ast
    };
    return ret;
}

bool MetaInfoTracker::runOnModule(llvm::Module& M) {
    using borealis::util::isValid;
    using borealis::util::takePtr;
    using borealis::util::view;
    using borealis::util::viewContainer;

    DebugInfoFinder dfi;
    dfi.processModule(M);

    auto& sm = GetAnalysis<sm_t>::doit(this).provide();
    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    auto* GlobalsDesc = M.getFunction(
         intrinsic_manager.getFuncName(
             function_type::INTRINSIC_GLOBAL_DESCRIPTOR_TABLE,
             ""
         )
    );

    for (CallInst* call : viewContainer(GlobalsDesc)
                         .flatten()
                         .map(takePtr())
                         .map(llvm::dyn_caster<CallInst>())
                         .filter(isValid())) {
        if (intrinsic_manager.getIntrinsicType(*call) == function_type::INTRINSIC_GLOBAL) {
            DIGlobalVariable glob(call->getMetadata("var"));
            auto* garg = call->getArgOperand(0);
            if (auto* load = dyn_cast<LoadInst>(garg)) {
                // if garg is a load, add it dereferenced
                vars.put(load->getPointerOperand(), mkVI(sm, glob, nullptr, true));
            } else {
                // else it has been optimized away, put it as-is
                vars.put(garg, mkVI(sm, glob));
            }
        }
    }

    for (auto& msp : view(dfi.subprogram_begin(), dfi.subprogram_end())) {
        if (!DIDescriptor(msp).isSubprogram()) continue;

        DISubprogram sp(msp);
        vars.put(sp.getFunction(), mkVI(sm, sp));
    }

    for (auto& I : viewContainer(M).flatten().flatten()) {

        if (DbgDeclareInst* inst = dyn_cast<DbgDeclareInst>(&I)) {
            auto* val = inst->getAddress();
            DIVariable var(inst->getVariable());

            clang::Decl* decl = nullptr;
            if (auto* inst = dyn_cast_or_null<Instruction>(val)) {
                if (auto* meta = inst->getMetadata("clang.decl.ptr")) {
                    if (auto* ival = dyn_cast_or_null<ConstantInt>(meta->getOperand(0))){
                        decl = reinterpret_cast<clang::Decl*>(ival->getValue().getZExtValue());
                    }
                }
            }

            auto vi = mkVI(sm, var, decl, true);

            // debug declare has additional location data attached through
            // dbg metadata
            if (auto* nodeloc = inst->getMetadata("dbg")) {
                DILocation dloc(nodeloc);

                for (auto& locus : vi.originalLocus) {
                    locus.loc.line = dloc.getLineNumber();
                    locus.loc.col = dloc.getColumnNumber();
                }
            }

            vars.put(val, vi);

        } else if (CallInst* inst = dyn_cast_or_null<CallInst>(&I)) {
            if (intrinsic_manager.getIntrinsicType(*inst) == function_type::INTRINSIC_VALUE) {
                auto* val = inst->getArgOperand(1);
                DIVariable var(inst->getMetadata("var"));

                auto vi = mkVI(sm, var);

                // debug value has additional location data attached through
                // dbg metadata
                if (auto* nodeloc = inst->getMetadata("dbg")) {
                    DILocation dloc(nodeloc);

                    for (auto& locus: vi.originalLocus) {
                        locus.loc.line = dloc.getLineNumber();
                        locus.loc.col = dloc.getColumnNumber();
                    }
                }

                vars.put(val, vi);
            } else if (intrinsic_manager.getIntrinsicType(*inst) == function_type::INTRINSIC_DECLARE) {
                auto* val = inst->getArgOperand(0);
                DIVariable var(inst->getMetadata("var"));

                auto vi = mkVI(sm, var, nullptr, true);

                // debug value has additional location data attached through
                // dbg metadata
                if (auto* nodeloc = inst->getMetadata("dbg")) {
                    DILocation dloc(nodeloc);

                    for (auto& locus: vi.originalLocus) {
                        locus.loc.line = dloc.getLineNumber();
                        locus.loc.col = dloc.getColumnNumber();
                    }
                }

                vars.put(val, vi);
            }
        }
    } // for (auto& I : viewContainer(M).flatten().flatten())

    return false;
}

void MetaInfoTracker::print(llvm::raw_ostream&, const llvm::Module*) const {
    for (const auto& var : vars) {
        infos() << " " << llvm::valueSummary(var.first) << " ==> " << var.second << endl;
    }
}

char MetaInfoTracker::ID;
static RegisterPass<MetaInfoTracker>
X("meta-tracker", "Meta info for values");

} /* namespace borealis */
