/*
 * llvm.cpp
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileManager.h>
#include <clang/Frontend/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Analysis/DIBuilder.h>
#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Type.h>

#include "Codegen/llvm.h"

namespace borealis {

void insertBeforeWithLocus(
        llvm::Instruction* what,
        llvm::Instruction* before,
        const Locus& loc) {
    what->insertBefore(before);
    setDebugLocusWithCopiedScope(what, before, loc);
}

void setDebugLocusWithCopiedScope(
        llvm::Instruction* to,
        llvm::Instruction* from,
        const Locus& loc) {
    using namespace llvm;

    auto& ctx = from->getContext();

    auto* dbg = MDNode::get(
            ctx,
            std::vector<Value*>{
                    ConstantInt::get(Type::getInt32Ty(ctx), loc.loc.line),
                    ConstantInt::get(Type::getInt32Ty(ctx), loc.loc.col),
                    from->getDebugLoc().getScope(ctx),
                    nullptr
            });
    to->setMetadata("dbg", dbg);
}

llvm::MDNode* ptr2MDNode(llvm::LLVMContext& ctx, void* ptr) {
    llvm::Constant* ptrAsInt = llvm::ConstantInt::get(
        ctx,
        llvm::APInt(sizeof(uintptr_t) * 8, reinterpret_cast<uintptr_t>(ptr))
    );
    return llvm::MDNode::get(ctx, ptrAsInt);
}

void* MDNode2Ptr(llvm::MDNode* ptr) {
    if (ptr)
        if (auto* i = llvm::dyn_cast<llvm::ConstantInt>(ptr->getOperand(0)))
            return reinterpret_cast<void*>(static_cast<uintptr_t>(i->getLimitedValue()));
    return nullptr;
}

std::string getRawSource(const clang::FileManager&, const LocusRange& lc) {
    // FIXME: You know what to do...

    return std::string{};
}

unsigned long long getTypeSizeInElems(llvm::Type* type) {
    using namespace llvm;
    using borealis::util::view;

    auto res = 0ULL;

    if (auto* structType = dyn_cast_or_null<StructType>(type)) {
        for (auto* structElem : view(structType->element_begin(), structType->element_end())) {
            res += getTypeSizeInElems(structElem);
        }
    } else if (auto* arrayType = dyn_cast_or_null<ArrayType>(type)) {
        res += arrayType->getArrayNumElements() * getTypeSizeInElems(arrayType->getArrayElementType());
    } else {
        res = 1ULL;
    }

    return res;
}

util::option<std::string> getAsCompileTimeString(llvm::Value* value) {
    using namespace llvm;

    auto* v = value->stripPointerCasts();
    if (auto* gv = dyn_cast<GlobalVariable>(v)) {
        if (gv->hasInitializer()) v = gv->getInitializer();
    }

    if (auto* cv = dyn_cast<ConstantDataSequential>(v)) {
        if (cv->isCString()) return util::just(cv->getAsCString().str());
        else if (cv->isString()) return util::just(cv->getAsString().str());
    }

    return util::nothing();
}

std::set<DIType>& flattenTypeTree(DIType di, std::set<DIType>& collected) {
    if(collected.count(di)) return collected;
    collected.insert(di);

    if(DICompositeType struct_ = di) {
        auto members = struct_.getTypeArray();
        for(size_t i = 0U; i < members.getNumElements(); ++i) {
            auto mem = members.getElement(i);
            flattenTypeTree(mem, collected);
        }
    } else if(DIDerivedType derived = di) {
        flattenTypeTree(derived.getTypeDerivedFrom(), collected);
    }

    return collected;
}

std::set<DIType> flattenTypeTree(DIType di) {
    std::set<DIType> collected;
    flattenTypeTree(di, collected);
    return std::move(collected);
}

DIType stripAliases(DIType tp) {
    if(DIAlias alias = tp) {
        return stripAliases(alias.getOriginal());
    } else return tp;
}

#include "Util/macros.h"
std::map<llvm::Type*, DIType>& flattenTypeTree(
    const std::pair<llvm::Type*, DIType>& tp,
    std::map<llvm::Type*, DIType>& collected) {

    if(collected.count(tp.first)) return collected;
    collected.insert(tp);

    if(DIAlias alias_ = tp.second) {
        flattenTypeTree(std::make_pair(tp.first, alias_.getOriginal()), collected);
    } else if(DICompositeType mstruct_ = tp.second) {
        auto mmembers = mstruct_.getTypeArray();
        ASSERTC(mmembers.getNumElements() == tp.first->getNumContainedTypes());

        for(size_t i = 0U; i < mmembers.getNumElements(); ++i) {
            auto mmem = mmembers.getElement(i);
            auto mem = tp.first->getContainedType(i);
            flattenTypeTree(std::make_pair(mem, mmem), collected);
        }
    } else if(DIDerivedType derived = tp.second) {
        flattenTypeTree(std::make_pair(tp.first->getContainedType(0), derived.getTypeDerivedFrom()), collected);
    }

    return collected;
}
#include "Util/unmacros.h"

std::map<llvm::Type*, DIType> flattenTypeTree(const std::pair<llvm::Type*, DIType>& tp) {
    std::map<llvm::Type*, DIType> collected;
    flattenTypeTree(tp, collected);
    return std::move(collected);
}

} // namespace borealis
