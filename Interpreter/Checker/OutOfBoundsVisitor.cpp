//
// Created by abdullin on 11/1/17.
//

#include "Interpreter/Domain/Memory/PointerDomain.hpp"
#include "Interpreter/Domain/Memory/ArrayDomain.hpp"
#include "Interpreter/Domain/Memory/StructDomain.hpp"
#include "Interpreter/Domain/Memory/FunctionDomain.hpp"
#include "Interpreter/Domain/Memory/MemoryLocation.hpp"
#include "OutOfBoundsVisitor.h"

#include "Util/collections.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

using RetTy = OutOfBoundsVisitor::RetTy;

RetTy OutOfBoundsVisitor::visit(AbstractDomain::Ptr domain, const std::vector<AbstractDomain::Ptr>& indices) {
    if (auto* array = llvm::dyn_cast<AbstractFactory::ArrayT>(domain.get()))
        return visitArray(*array, indices);
    else if (auto* function = llvm::dyn_cast<AbstractFactory::FunctionT>(domain.get()))
        return visitFunction(*function, indices);
    else if (auto* pointer = llvm::dyn_cast<AbstractFactory::PointerT>(domain.get()))
        return visitPointer(*pointer, indices);
    else if (auto* structure = llvm::dyn_cast<AbstractFactory::StructT>(domain.get()))
        return visitStruct(*structure, indices);
    else if (auto* arrayLoc = llvm::dyn_cast<AbstractFactory::ArrayLocationT>(domain.get()))
        return visitArrayLocation(*arrayLoc, indices);
    else if (auto* structLoc = llvm::dyn_cast<AbstractFactory::StructLocationT>(domain.get()))
        return visitStructLocation(*structLoc, indices);
    else if (auto* functionLoc = llvm::dyn_cast<AbstractFactory::FunctionLocationT>(domain.get()))
        return visitFunctionLocation(*functionLoc, indices);
    else if (auto* nullLoc = llvm::dyn_cast<AbstractFactory::NullLocationT>(domain.get()))
        return visitNullLocation(*nullLoc, indices);
    else
        UNREACHABLE("Unknown domain type")
}

RetTy OutOfBoundsVisitor::visitArray(const AbstractFactory::ArrayT& array, const std::vector<AbstractDomain::Ptr>& indices) {
    if (array.isTop() || array.isBottom()) return true;

    auto length = array.length();
    auto bounds = af_->unsignedBounds(indices.front());

    if (bounds.first > length || bounds.second > length) {
        return true;
    }

    std::vector<AbstractDomain::Ptr> sub_idx(indices.begin() + 1, indices.end());
    if (sub_idx.empty()) return false;
    for (auto i = bounds.first; i <= bounds.second; ++i) {
        auto&& element = array.elements().find((size_t) i);
        if (element == array.elements().end()) continue;
        if (visit((*element).second, sub_idx)) return true;
    }

    return false;
}

RetTy OutOfBoundsVisitor::visitFunction(const AbstractFactory::FunctionT&, const std::vector<AbstractDomain::Ptr>&) {
    return false;
}

RetTy OutOfBoundsVisitor::visitPointer(const AbstractFactory::PointerT& ptr, const std::vector<AbstractDomain::Ptr>& indices) {
    if (ptr.isTop() || ptr.isBottom()) return true;


    for (auto&& it : ptr.locations()) {
        if (visit(it, indices)) return true;
    }

    return false;
}

RetTy OutOfBoundsVisitor::visitStruct(const AbstractFactory::StructT& structure, const std::vector<AbstractDomain::Ptr>& indices) {
    if (structure.isTop() || structure.isBottom()) return true;

    auto length = structure.size();
    auto bounds = af_->unsignedBounds(indices.front());

    ASSERT(bounds.first == bounds.second, "trying to load from nondetermined index of struct");
    auto index = (size_t) bounds.first;

    if (index > length) {
        return true;
    }

    std::vector<AbstractDomain::Ptr> sub_idx(indices.begin() + 1, indices.end());
    if (not sub_idx.empty()) {
        return visit(structure.elements()[index], sub_idx);
    }

    return false;
}

RetTy OutOfBoundsVisitor::visitArrayLocation(const AbstractFactory::ArrayLocationT& arrayLoc, const std::vector<AbstractDomain::Ptr>& indices) {
    if (arrayLoc.isTop() || arrayLoc.isBottom()) return true;

    AbstractDomain::Ptr offset = *arrayLoc.offsets().begin();
    auto&& newIndices = std::vector<AbstractDomain::Ptr>(indices.begin(), indices.end());
    newIndices[0] = newIndices[0] + offset;

    return visit(arrayLoc.base(), newIndices);
}

RetTy OutOfBoundsVisitor::visitStructLocation(const AbstractFactory::StructLocationT& structLoc, const std::vector<AbstractDomain::Ptr>& indices) {
    if (structLoc.isTop() || structLoc.isBottom()) return true;

    auto&& newIndices = std::vector<AbstractDomain::Ptr>(indices.begin(), indices.end());
    auto&& originalFirst = newIndices[0];
    for (auto&& it : structLoc.offsets()) {
        newIndices[0] = originalFirst + it;
        if (visit(structLoc.base(), newIndices)) return true;
    }

    return false;
}

RetTy OutOfBoundsVisitor::visitFunctionLocation(const AbstractFactory::FunctionLocationT&, const std::vector<AbstractDomain::Ptr>&) {
    return false;
}

RetTy OutOfBoundsVisitor::visitNullLocation(const AbstractFactory::NullLocationT&, const std::vector<AbstractDomain::Ptr>&) {
    return true;
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"