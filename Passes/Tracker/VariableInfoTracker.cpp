/*
 * VariableInfoTracker.cpp
 *
 *  Created on: Jan 11, 2013
 *      Author: belyaev
 */

#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

#include "Codegen/intrinsics_manager.h"
#include "Logging/logger.hpp"
#include "Passes/Tracker/VariableInfoTracker.h"
#include "Util/functional.hpp"
#include "Util/passes.hpp"

namespace borealis {

VariableInfoTracker::VariableInfoTracker() : ModulePass(ID) {}

VariableInfoTracker::~VariableInfoTracker() {}

void VariableInfoTracker::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<sm_t>::addRequiredTransitive(AU);
}

using namespace llvm;
using borealis::util::option;
using borealis::util::nothing;
using borealis::util::just;

static VarInfo mkVI(const clang::FileManager&, const llvm::DISubprogram& node,
        const borealis::DebugInfoFinder&,
        clang::Decl* ast = nullptr, bool allocated = false) {
    VarInfo ret{
        just(node.getName().str()),
        just(Locus(node.getFilename().str(), node.getLineNumber(), 0U)),
        allocated ? VarInfo::Allocated : VarInfo::Plain,
        node.getType(),
        ast
    };
    return ret;
}

static VarInfo mkVI(const clang::FileManager&, const llvm::DIGlobalVariable& node,
        const borealis::DebugInfoFinder& context,
        clang::Decl* ast = nullptr, bool allocated = false) {
    VarInfo ret{
        just(node.getName().str()),
        just(Locus(node.getFilename().str(), node.getLineNumber(), 0U)),
        allocated ? VarInfo::Allocated : VarInfo::Plain,
        stripAliases(context, context.resolve(node.getType())),
        ast
    };
    return ret;
}

static VarInfo mkVI(const clang::FileManager&, const llvm::DIVariable& node,
        const borealis::DebugInfoFinder& context,
        clang::Decl* ast = nullptr, bool allocated = false) {
    VarInfo ret{
        just(node.getName().str()),
        just(Locus(node.getContext().getFilename(), node.getLineNumber(), 0U)),
        allocated ? VarInfo::Allocated : VarInfo::Plain,
        stripAliases(context, context.resolve(node.getType())),
        ast
    };
    return ret;
}

static VarInfo mkVI(const clang::FileManager&, const DIBorealisVarDesc& desc,
        const borealis::DebugInfoFinder&,
        clang::Decl* ast = nullptr, bool allocated = false) {
    VarInfo ret{
        just(desc.getVarName().str()),
        just(Locus(desc.getFileName().str(), desc.getLine(), desc.getCol())),
        allocated ? VarInfo::Allocated : VarInfo::Plain,
        DIType{},
        ast
    };
    return ret;
}

bool VariableInfoTracker::runOnModule(llvm::Module& M) {
    using borealis::ops::take_pointer;
    using borealis::util::view;
    using borealis::util::viewContainer;

    ctx = &M.getContext();
    m = &M;

    dfi = DebugInfoFinder{};
    dfi.processModule(M);

    CTF.processTypes(dfi);

    auto& sm = GetAnalysis<sm_t>::doit(this).provide();
    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    auto* GlobalsDesc = M.getFunction(
         intrinsic_manager.getFuncName(
             function_type::INTRINSIC_GLOBAL_DESCRIPTOR_TABLE,
             ""
         )
    );

    for (CallInst* call : viewContainer(*GlobalsDesc)
                         .flatten()
                         .map(take_pointer)
                         .map(llvm::dyn_caster<CallInst>())
                         .filter()) {
        if (intrinsic_manager.getIntrinsicType(*call) == function_type::INTRINSIC_GLOBAL) {
            DIGlobalVariable glob(call->getMetadata("var"));
            auto* garg = call->getArgOperand(0);
            if (auto* load = dyn_cast<LoadInst>(garg)) {
                // if garg is a load, add it dereferenced
                vars.put(load->getPointerOperand(), mkVI(sm, glob, dfi, nullptr, true));
            } else {
                // else it has been optimized away, put it as-is
                vars.put(garg, mkVI(sm, glob, dfi));
            }
        }
    }

    for (auto& msp : dfi.subprograms()) {
        if (!DIDescriptor(msp).isSubprogram()) continue;

        DISubprogram sp(msp);
        // FIXME: this is generally fucked up...
        if (auto f = sp.getFunction()) {
            vars.put(f, mkVI(sm, sp, dfi, nullptr, true));
        }
    }

// FIXME: decide whether we want this or not
//    {
//        auto clangGlobals = M.getNamedMetadata("bor.global.decls");
//        auto clangGlobalsView =
//            util::range(0U, clangGlobals->getNumOperands())
//           .map([clangGlobals](size_t i){ return clangGlobals->getOperand(i); });
//        for(auto md : clangGlobalsView) {
//            if(DIBorealisVarDesc clangDesc = md) {
//                vars.put(clangDesc.getGlobal(), mkVI(sm, clangDesc));
//            }
//        }
//    }

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

            auto vi = mkVI(sm, var, dfi, decl, true);

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

                auto vi = mkVI(sm, var, dfi, nullptr, isAllocaLikeValue(val));

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

                auto vi = mkVI(sm, var, dfi, nullptr, true);

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

void VariableInfoTracker::print(llvm::raw_ostream&, const llvm::Module*) const {
    for (const auto& var : vars) {
        infos() << " " << llvm::valueSummary(var.first) << " ==> " << var.second << endl;
    }
}

char VariableInfoTracker::ID;
static RegisterPass<VariableInfoTracker>
X("meta-tracker", "Meta info for values");

} /* namespace borealis */
