/*
 * ClangDeclTrackerPass.cpp
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

namespace borealis {

MetaInfoTrackerPass::MetaInfoTrackerPass(): ModulePass(ID) {}

MetaInfoTrackerPass::~MetaInfoTrackerPass() {}

void MetaInfoTrackerPass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
    Info.addRequired<sm_t>();
    Info.addRequired<loops>();
    Info.setPreservesAll();
}

using borealis::util::option;
using borealis::util::nothing;
using borealis::util::just;
using borealis::VarInfo;
using borealis::Locus;
using llvm::dyn_cast_or_null;
using llvm::DIVariable;
using llvm::DIDescriptor;

static VarInfo mkVI(const clang::SourceManager& sm, clang::Decl* ast = nullptr, bool allocated = false) {
    if (clang::NamedDecl* decl = llvm::dyn_cast_or_null<clang::NamedDecl>(ast)) {
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

    using llvm::inst_begin;
    using llvm::inst_end;
    using llvm::dyn_cast;
    using llvm::dyn_cast_or_null;
    using llvm::DbgDeclareInst;
    using llvm::DbgValueInst;
    using llvm::CallInst;
    using llvm::MDNode;
    using llvm::Instruction;
    using llvm::ConstantInt;

    llvm::DebugInfoFinder dfi;
    dfi.processModule(M);

    const clang::SourceManager& sm =
            getAnalysis< DataProvider<clang::SourceManager> >().provide();

    for (auto& mglob: view(dfi.global_variable_begin(), dfi.global_variable_end())) {
        llvm::DIDescriptor di(mglob);
        if (!di.isGlobalVariable()) continue;

        llvm::DIGlobalVariable glob(mglob);
        // FIXME: deal with the globals that get optimized out
        if(glob.getGlobal()) vars.put(glob.getGlobal(), mkVI(sm, glob));
    }

    for (auto& msp: view(dfi.subprogram_begin(), dfi.subprogram_end())) {
        llvm::DIDescriptor di(msp);
        if(!di.isSubprogram()) continue;

        llvm::DISubprogram sp(msp);
        vars.put(sp.getFunction(), mkVI(sm, sp));
    }

    for (auto& F: M) {
        for (auto& I: view(inst_begin(F), inst_end(F))) {

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

                // debug declare has additional location data attached through dbg metadata
                if (auto* nodeloc = inst->getMetadata("dbg")) {
                    llvm::DILocation dloc(nodeloc);

                    for (auto& locus: vi.originalLocus) {
                        locus.loc.line = dloc.getLineNumber();
                        locus.loc.col = dloc.getColumnNumber();
                    }
                }

                vars.put(val, vi);

            } else if (CallInst* inst = dyn_cast_or_null<CallInst>(&I)) {
                if(IntrinsicsManager::getInstance().getIntrinsicType(*inst)
                        == function_type::INTRINSIC_VALUE) {
                    auto* val = inst->getArgOperand(1);
                    DIVariable var(inst->getMetadata("var"));

                    clang::Decl* decl = nullptr;
                    auto vi = mkVI(sm, var, decl);

                    // debug value has additional location data attached through dbg metadata
                    if (auto* nodeloc = inst->getMetadata("dbg")) {
                        llvm::DILocation dloc(nodeloc);

                        for (auto& locus: vi.originalLocus) {
                            locus.loc.line = dloc.getLineNumber();
                            locus.loc.col = dloc.getColumnNumber();
                        }
                    }

                    vars.put(val, vi);
                }
            }
        } // for (auto& I: view(inst_begin(F), inst_end(F)))
    } // for (auto& F: M)

    return false;
}

inline std::string compact_string(llvm::Value* val) {
    using llvm::dyn_cast_or_null;
    using borealis::util::toString;

    if(auto* f = dyn_cast_or_null<llvm::Function>(val)) {
        return toString(f->getName());
    } else if (auto* bb = dyn_cast_or_null<llvm::BasicBlock>(val)) {
        return toString(bb->getName());
    } else return toString(*val);
}

void MetaInfoTrackerPass::print(llvm::raw_ostream&, const llvm::Module*) const {
    for(auto& var: vars) {
        infos() << compact_string(var.first) << " ==> " << var.second << endl;
    }
}

char MetaInfoTrackerPass::ID = 0U;

static llvm::RegisterPass<MetaInfoTrackerPass>
X("meta-tracker", "Meta info for values");

} /* namespace borealis */
