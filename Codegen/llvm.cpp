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

llvm::StringRef getRawSource(const clang::SourceManager& sm, const LocusRange& range) {
    using namespace clang;

    FileManager& fm = sm.getFileManager();

    if (range.lhv.isUnknown() || range.rhv.isUnknown()) {
        return "Encountered unknown location, enjoy a smiley =)";
    }

    const FileEntry* begFile = fm.getFile(range.lhv.filename);

    auto begLoc = sm.translateFileLineCol(begFile, range.lhv.loc.line, range.lhv.loc.col);
    auto endLoc = sm.translateFileLineCol(begFile, range.rhv.loc.line, range.rhv.loc.col);

    clang::FileID BeginFileID;
    clang::FileID EndFileID;
    unsigned BeginOffset;
    unsigned EndOffset;

    std::tie(BeginFileID, BeginOffset) = sm.getDecomposedLoc(begLoc);
    std::tie(EndFileID, EndOffset) = sm.getDecomposedLoc(endLoc);

    bool Invalid = false;
    const char* BufferStart = sm.getBufferData(BeginFileID, &Invalid).data();

    return StringRef{ BufferStart + BeginOffset, EndOffset - BeginOffset };
}

unsigned long long getTypeSizeInElems(llvm::Type* type) {
    using namespace llvm;
    using borealis::util::view;

    unsigned long long res = 0;

    if (auto* structType = dyn_cast_or_null<StructType>(type)) {
        for (auto* structElem : view(structType->element_begin(), structType->element_end())) {
            res += getTypeSizeInElems(structElem);
        }
    } else if (auto* arrayType = dyn_cast_or_null<ArrayType>(type)) {
        res += arrayType->getArrayNumElements() * getTypeSizeInElems(arrayType->getArrayElementType());
    } else {
        res = 1;
    }

    return res;
}

} // namespace borealis
