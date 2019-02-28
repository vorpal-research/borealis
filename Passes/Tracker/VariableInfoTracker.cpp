/*
 * VariableInfoTracker.cpp
 *
 *  Created on: Jan 11, 2013
 *      Author: belyaev
 */

#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <Codegen/CType/CTypeInfer.h>

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
    AUX<ext_vars_t>::addRequiredTransitive(AU);
}

using namespace llvm;
using borealis::util::option;
using borealis::util::nothing;
using borealis::util::just;

static VarInfo mkVI(
        CTypeFactory& CTF,
        const clang::FileManager&,
        const llvm::DISubprogram& node,
        borealis::DebugInfoFinder& DFI,
        bool allocated = false) {
    return VarInfo(
        node.getName().str(),
        Locus{ node.getFilename().str(), node.getLineNumber(), 0U },
        allocated ? StorageSpec::Memory : StorageSpec::Register,
        CTF.getRef(CTF.getType(node.getType(), DFI)),
        VariableKind::Global
    );
}

static VarInfo mkVI(
        CTypeFactory& CTF,
        const clang::FileManager&,
        const llvm::DIGlobalVariable& node,
        borealis::DebugInfoFinder& DFI,
        bool allocated = false) {
    return VarInfo{
        node.getName().str(),
        Locus(node.getFilename().str(), node.getLineNumber(), 0U),
        allocated ? StorageSpec::Memory : StorageSpec::Register,
        CTF.getRef(CTF.getType(DFI.resolve(node.getType()), DFI)),
        VariableKind::Global
    };
}

static VarInfo mkVI(
        CTypeFactory& CTF,
        const clang::FileManager&,
        const llvm::DIVariable& node,
        borealis::DebugInfoFinder& DFI,
        bool allocated = false) {
    VarInfo ret{
        node.getName().str(),
        Locus(node.getFile().getName().str(), node.getLineNumber(), 0U),
        allocated ? StorageSpec::Memory : StorageSpec::Register,
        CTF.getRef(CTF.getType(DFI.resolve(node.getType()), DFI)),
        VariableKind::Local
    };
    return ret;
}

//static VarInfo mkVI(const clang::FileManager&, const DIBorealisVarDesc& desc,
//        const borealis::DebugInfoFinder&,
//        clang::Decl* ast = nullptr, bool allocated = false) {
//    VarInfo ret{
//        desc.getVarName().str(),
//        Locus(desc.getFileName().str(), desc.getLine(), desc.getCol()),
//        allocated ? StorageSpec::Memory : StorageSpec::Register,
//        nullptr,
//        VariableKind::Global
//    };
//    return ret;
//}

bool VariableInfoTracker::runOnModule(llvm::Module& M) {
    using borealis::ops::take_pointer;
    using borealis::util::view;
    using borealis::util::viewContainer;

    ctx = &M.getContext();
    m = &M;

    dfi = DebugInfoFinder{};
    dfi.processModule(M);

    auto& sm = GetAnalysis<sm_t>::doit(this).provide();
    auto& ext_vars = GetAnalysis<ext_vars_t>::doit(this).provide();
    CTF = util::make_unique<CTypeFactory>(ext_vars.types);

    std::unordered_map<std::string, VarInfo> extVarByName;
    // FIXME: this works for now, but in general is fucked up
    // we should find a way to deal with multiple symbols with same name
    for(auto&& extv : ext_vars.vars) if(extv.kind != VariableKind::Local) extVarByName.emplace(extv.name, extv);

    // FIXME: we need to store function and global info separately
    for(auto&& V : M.getFunctionList()) {
        auto&& vext = util::at(extVarByName, V.getName());
        if(vext) vars.put(&V, vext.getUnsafe());
    }

    for(auto&& V : M.getGlobalList()) {
        auto vext = util::at(extVarByName, V.getName());
        if(vext) {
            auto&& resolve = vext.getUnsafe();
            resolve.storage = StorageSpec::Memory;
            vars.put(&V, std::move(resolve));
        }
    }

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
                vars.put(load->getPointerOperand(), mkVI(*CTF, sm, glob, dfi, true));
            } else {
                // else it has been optimized away, put it as-is
                vars.put(garg, mkVI(*CTF, sm, glob, dfi));
            }
        }
    }

    for (auto& msp : dfi.subprograms()) {
        if (!DIDescriptor(msp).isSubprogram()) continue;

        DISubprogram sp(msp);
        // FIXME: this is generally fucked up...
        if (auto f = sp.getFunction()) {
            vars.put(f, mkVI(*CTF, sm, sp, dfi, true));
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
//                // FIXME: think of something better for these guys
//                if(clangDesc.getGlobal() && clangDesc.getGlobal()->isExternalLinkage(clangDesc.getGlobal()->getLinkage()))
//                    vars.put(clangDesc.getGlobal(), mkVI(CTF, sm, clangDesc, dfi));
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

            auto&& llvmType = val->getType();
            auto&& metaType = dfi.resolve(var.getType());

            auto vi = mkVI(*CTF, sm, var, dfi, isAllocaLikeTypes(llvmType, metaType, dfi));

            // debug declare has additional location data attached through
            // dbg metadata
            if (auto* nodeloc = inst->getMetadata("dbg")) {
                DILocation dloc(nodeloc);

                vi.locus = dloc;
            }

            vars.put(val, vi);

        } else if (CallInst* inst = dyn_cast_or_null<CallInst>(&I)) {
            if (intrinsic_manager.getIntrinsicType(*inst) == function_type::INTRINSIC_VALUE) {
                auto* val = inst->getArgOperand(1);
                DIVariable var(inst->getMetadata("var"));

                if (not var) continue;

                auto&& llvmType = val->getType();
                auto&& metaType = dfi.resolve(var.getType());

                auto vi = mkVI(*CTF, sm, var, dfi, isAllocaLikeTypes(llvmType, metaType, dfi));

                // debug value has additional location data attached through
                // dbg metadata
                if (auto* nodeloc = inst->getMetadata("dbg")) {
                    DILocation dloc(nodeloc);

                    vi.locus = dloc;
                }

                vars.put(val, vi);

            } else if (intrinsic_manager.getIntrinsicType(*inst) == function_type::INTRINSIC_DECLARE) {
                auto* val = inst->getArgOperand(0);
                DIVariable var(inst->getMetadata("var"));

                if (not var) continue;

                auto&& llvmType = val->getType();
                auto&& metaType = dfi.resolve(var.getType());

                auto vi = mkVI(*CTF, sm, var, dfi, isAllocaLikeTypes(llvmType, metaType, dfi));

                // debug value has additional location data attached through
                // dbg metadata
                if (auto* nodeloc = inst->getMetadata("dbg")) {
                    DILocation dloc(nodeloc);
                    vi.locus = dloc;
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
