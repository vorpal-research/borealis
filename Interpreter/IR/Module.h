//
// Created by abdullin on 3/2/17.
//

#ifndef BOREALIS_MODULE_H
#define BOREALIS_MODULE_H

#include <map>
#include <string>
#include <unordered_map>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <Passes/Tracker/SlotTrackerPass.h>

#include "Interpreter/IR/Function.h"
#include "Interpreter/Domain/DomainFactory.h"
#include "Interpreter/Domain/GlobalManager.hpp"
#include "Interpreter/Domain/VariableFactory.hpp"

namespace borealis {
namespace absint {
namespace ir {

class Module {
public:

    using GlobalsMap = std::map<const llvm::Value*, AbstractDomain::Ptr>;
    using FunctionMap = std::unordered_map<const llvm::Function*, Function::Ptr>;

    Module(const llvm::Module* module, SlotTrackerPass* st, bool initAddrTakenFuncs = true);

    std::vector<Function::Ptr> roots();
    Function::Ptr get(const llvm::Function* function);
    Function::Ptr get(const std::string& fname);

    bool checkVisited(const llvm::Value* val) const;
    const llvm::Module* instance() const;
    GlobalManager* globalManager();
    const GlobalManager* globalManager() const;
    AbstractDomain::Ptr global(const llvm::Value* value) const;
    GlobalsMap globalsFor(Function::Ptr f) const;
    GlobalsMap globalsFor(const BasicBlock* bb) const;
    const FunctionMap& functions() const;
    const FunctionMap& addressTakenFunctions() const;
    /// Returns vector of address taken functions of a given prototype
    /// Prototype should NOT be a VarArg function
    std::vector<Function::Ptr> findByPrototype(const llvm::Type* prototype) const;

    SlotTrackerPass* slotTracker() const;
    VariableFactory* variableFactory() const;

    std::string toString() const;

    AbstractDomain::Ptr getDomainFor(const llvm::Value* value, const llvm::BasicBlock* location);

    void initAddressTakenFunctions();
    void initGlobals(const std::unordered_set<const llvm::Value*>& globals);

private:

    const llvm::Module* instance_;
    SlotTrackerPass* ST_;
    GlobalManager gm_;
    VariableFactory vf_;
    FunctionMap functions_;
    FunctionMap addressTakenFunctions_;

};

std::ostream& operator<<(std::ostream& s, const Module& m);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m);

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_MODULE_H
