/*
 * MetaInfoTrackerPass.cpp
 *
 *  Created on: Jan 11, 2013
 *      Author: belyaev
 */

#include <llvm/Constants.h>
#include <llvm/Instruction.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Support/InstIterator.h>

#include "Codegen/intrinsics_manager.h"
#include "Logging/logger.hpp"
#include "Passes/MetaInfoTrackerPass.h"
#include "Util/util.h"
#include "Util/iterators.hpp"

namespace borealis {

MetaInfoTrackerPass::MetaInfoTrackerPass() : ModulePass(ID) {}

MetaInfoTrackerPass::~MetaInfoTrackerPass() {}

void MetaInfoTrackerPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<sm_t>::addRequiredTransitive(AU);
}

using borealis::util::option;
using borealis::util::nothing;
using borealis::util::just;

using borealis::Locus;
using borealis::VarInfo;

using llvm::dyn_cast;
using llvm::dyn_cast_or_null;
using llvm::DIDescriptor;
using llvm::DIGlobalVariable;
using llvm::DILocation;
using llvm::DISubprogram;
using llvm::DIVariable;

static VarInfo mkVI(const clang::SourceManager& sm,
        clang::Decl* ast = nullptr, bool allocated = false) {
    if (clang::NamedDecl* decl = dyn_cast_or_null<clang::NamedDecl>(ast)) {
        return VarInfo {
            just(decl->getName().str()),
            just(Locus(sm.getPresumedLoc(ast->getLocation()))),
            allocated ? VarInfo::Allocated : VarInfo::Plain,
            ast
        };
    }
    return VarInfo();
}

static VarInfo mkVI(const clang::SourceManager& sm, const llvm::DISubprogram& node,
        clang::Decl* ast = nullptr, bool allocated = false) {
    VarInfo ret {
        just(node.getName().str()),
        just(Locus(node.getFilename().str(), node.getLineNumber(), 0U)),
        allocated ? VarInfo::Allocated : VarInfo::Plain,
        ast
    };
    return ret.overwriteBy(mkVI(sm, ast));
}

static VarInfo mkVI(const clang::SourceManager& sm, const llvm::DIGlobalVariable& node,
        clang::Decl* ast = nullptr, bool allocated = false) {
    VarInfo ret {
        just(node.getName().str()),
        just(Locus(node.getFilename().str(), node.getLineNumber(), 0U)),
        allocated ? VarInfo::Allocated : VarInfo::Plain,
        ast
    };
    return ret.overwriteBy(mkVI(sm, ast));
}

static VarInfo mkVI(const clang::SourceManager& sm, const llvm::DIVariable& node,
        clang::Decl* ast = nullptr, bool allocated = false) {
    VarInfo ret {
        just(node.getName().str()),
        just(Locus(node.getContext().getFilename(), node.getLineNumber(), 0U)),
        allocated ? VarInfo::Allocated : VarInfo::Plain,
        ast
    };
    return ret.overwriteBy(mkVI(sm, ast));
}

bool MetaInfoTrackerPass::runOnModule(llvm::Module& M) {
    using borealis::util::view;
    using borealis::util::viewContainer;

    using llvm::inst_begin;
    using llvm::inst_end;
    using llvm::CallInst;
    using llvm::ConstantInt;
    using llvm::DbgDeclareInst;
    using llvm::DbgValueInst;
    using llvm::Instruction;
    using llvm::LoadInst;
    using llvm::MDNode;

    TRACE_FUNC;

    llvm::DebugInfoFinder dfi;
    dfi.processModule(M);

    const clang::SourceManager& sm =
            getAnalysis<sm_t>().provide();
    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    auto* GlobalsDesc = M.getFunction(
         intrinsic_manager.getFuncName(
             function_type::INTRINSIC_GLOBAL_DESCRIPTOR_TABLE,
             ""
         )
    );

    for (CallInst* call : viewContainer(GlobalsDesc)
                         .flatten()
                         .map(borealis::util::takePtr())
                         .map(llvm::dyn_caster<CallInst>())
                         .filter()) {
        if (intrinsic_manager.getIntrinsicType(*call) == function_type::INTRINSIC_GLOBAL) {
            DIGlobalVariable glob(call->getMetadata("var"));
            auto* garg = call->getArgOperand(0);
            // if garg is a load, add it dereferenced
            if (auto* load = dyn_cast<LoadInst>(garg)) {
                vars.put(load->getPointerOperand(), mkVI(sm, glob, nullptr, true));
            // else it has been optimized away, put it as-is
            } else {
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

        if (DbgDeclareInst* inst = dyn_cast_or_null<DbgDeclareInst>(&I)) {
            auto* val = inst->getAddress();
            DIVariable var(inst->getVariable());

            clang::Decl* decl = nullptr;
            if (Instruction* inst = dyn_cast_or_null<Instruction>(val)) {
                if (MDNode* meta = inst->getMetadata("clang.decl.ptr")) {
                    if (ConstantInt* ival = dyn_cast_or_null<ConstantInt>(meta->getOperand(0))){
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
            if (IntrinsicsManager::getInstance().getIntrinsicType(*inst) == function_type::INTRINSIC_VALUE) {
                auto* val = inst->getArgOperand(1);
                DIVariable var(inst->getMetadata("var"));

                clang::Decl* decl = nullptr;
                auto vi = mkVI(sm, var, decl);

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
    } // for (auto& I : flat2View(M.begin(), M.end()))


    return false;
}

void MetaInfoTrackerPass::print(llvm::raw_ostream&, const llvm::Module*) const {
    for (auto& var : vars) {
        infos() << " " << llvm::valueSummary(var.first) << " ==> " << var.second << endl;
    }
}

char MetaInfoTrackerPass::ID;
static RegisterPass<MetaInfoTrackerPass>
X("meta-tracker", "Meta info for values");

} /* namespace borealis */
