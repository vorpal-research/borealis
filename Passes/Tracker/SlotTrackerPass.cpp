/*
 * SlotTrackerPass.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#include "Passes/Tracker/SlotTrackerPass.h"
#include "Util/passes.hpp"


namespace borealis {

bool SlotTrackerPass::doInitialization(llvm::Module&) {
    return false;
}

bool SlotTrackerPass::doFinalization(llvm::Module& M) {
    globals.reset();
    funcs.clear();
    return false;
}

bool SlotTrackerPass::runOnModule(llvm::Module& M) {
    globals.reset(new SlotTracker(&M));
    types.incorporateTypes(M);
    for (auto& F : M)
        funcs[&F] = ptr_t{ new SlotTracker(globals.get(), &F) };

    return false;
}

void SlotTrackerPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
}

SlotTracker* SlotTrackerPass::getSlotTracker(const llvm::Function* func) const {
    if (func && borealis::util::containsKey(funcs, func)) {
        return funcs.at(func).get();
    } else {
        return nullptr;
    }
}

SlotTracker* SlotTrackerPass::getSlotTracker (const llvm::Module*) const{
    return globals.get();
}

SlotTracker* SlotTrackerPass::getSlotTracker (const llvm::BasicBlock* bb) const{
    return getSlotTracker(bb->getParent());
}

SlotTracker* SlotTrackerPass::getSlotTracker (const llvm::Instruction* inst) const{
    return getSlotTracker(inst->getParent());
}

SlotTracker* SlotTrackerPass::getSlotTracker (const llvm::Argument* arg) const{
    return getSlotTracker(arg->getParent());
}

SlotTracker* SlotTrackerPass::getSlotTracker (const llvm::Value* arg) const {
    if(auto dc = dyn_cast<llvm::Function>(arg)) return getSlotTracker(*dc);
    if(auto dc = dyn_cast<llvm::BasicBlock>(arg)) return getSlotTracker(*dc);
    if(auto dc = dyn_cast<llvm::GlobalValue>(arg)) return getSlotTracker(dc->getParent());
    if(auto dc = dyn_cast<llvm::Instruction>(arg)) return getSlotTracker(*dc);

    // default
    return getSlotTracker(static_cast<const llvm::Module*>(nullptr));
}

void SlotTrackerPass::printValue(const llvm::Value* v, llvm::raw_ostream& ost) const {
    ::borealis::printValue(const_cast<llvm::Value*>(v), ost, *getSlotTracker(v), const_cast<TypePrinting&>(types));
}

void SlotTrackerPass::printType(const llvm::Type* t, llvm::raw_ostream& ost) const {
    ::borealis::printType(const_cast<llvm::Type*>(t), ost, const_cast<TypePrinting&>(types));
}

std::string SlotTrackerPass::toString(const llvm::Value* v) const {
    std::string ret;
    llvm::raw_string_ostream rso(ret);
    printValue(v, rso);
    return std::move(rso.str());
}

std::string SlotTrackerPass::toString(const llvm::Type* t) const {
    std::string ret;
    llvm::raw_string_ostream rso(ret);
    printType(t, rso);
    return std::move(rso.str());
}

SlotTracker* SlotTrackerPass::getSlotTracker (const llvm::Function& func) const{
    return getSlotTracker(&func);
}

TypePrinting* SlotTrackerPass::getTypePrinting () const {
    return const_cast<TypePrinting*>(&types);
}


SlotTracker* SlotTrackerPass::getSlotTracker (const llvm::Module&) const{
    return globals.get();
}

SlotTracker* SlotTrackerPass::getSlotTracker (const llvm::BasicBlock& bb) const{
    return getSlotTracker(&bb);
}

SlotTracker* SlotTrackerPass::getSlotTracker (const llvm::Instruction& inst) const{
    return getSlotTracker(&inst);
}

SlotTracker* SlotTrackerPass::getSlotTracker (const llvm::Argument& arg) const{
    return getSlotTracker(&arg);
}

char SlotTrackerPass::ID;
static RegisterPass<SlotTrackerPass>
X("slot-tracker", "Provides slot tracker functionality for other passes");

} // namespace borealis
