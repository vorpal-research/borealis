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

namespace borealis {
namespace absint {

class Module {
public:

    using GlobalsMap = std::map<const llvm::Value*, Domain::Ptr>;
    using FunctionMap = std::unordered_map<const llvm::Function*, Function::Ptr>;

    Module(const llvm::Module* module, SlotTrackerPass* st);

    std::vector<Function::Ptr> getRootFunctions();
    Function::Ptr get(const llvm::Function* function);
    Function::Ptr get(const std::string& fname);

    bool checkVisited(const llvm::Value* val) const;
    const llvm::Module* getInstance() const;
    GlobalVariableManager* getGlobalVariableManager();
    const GlobalVariableManager* getGlobalVariableManager() const;
    Domain::Ptr findGlobal(const llvm::Value* value) const;
    GlobalsMap getGlobalsFor(Function::Ptr f) const;
    GlobalsMap getGlobalsFor(const BasicBlock* bb) const;
    const FunctionMap& getFunctions() const;
    const FunctionMap& getAddressTakenFunctions() const;
    /// Returns vector of address taken functions of a given prototype
    /// Prototype should NOT be a VarArg function
    std::vector<Function::Ptr> findFunctionsByPrototype(const llvm::Type* prototype) const;

    SlotTrackerPass* getSlotTracker() const;
    DomainFactory* getDomainFactory();

    std::string toString() const;

    Domain::Ptr getDomainFor(const llvm::Value* value, const llvm::BasicBlock* location);

    void initGlobals(const std::unordered_set<const llvm::Value*>& globals);

private:

    const llvm::Module* instance_;
    SlotTrackerPass* ST_;
    GlobalVariableManager GVM_;
    DomainFactory factory_;
    FunctionMap functions_;
    FunctionMap addressTakenFunctions_;

};

std::ostream& operator<<(std::ostream& s, const Module& m);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m);

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_MODULE_H
