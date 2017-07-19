//
// Created by abdullin on 3/2/17.
//

#ifndef BOREALIS_MODULE_H
#define BOREALIS_MODULE_H

#include <string>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <Passes/Tracker/SlotTrackerPass.h>

#include "Function.h"
#include "Interpreter/Domain/DomainFactory.h"

namespace borealis {
namespace absint {

class Module {
public:

    using GlobalsMap = std::map<const llvm::Value*, Domain::Ptr>;
    using FunctionMap = std::map<const llvm::Function*, Function::Ptr>;

    Module(const llvm::Module* module, SlotTrackerPass* st);

    Function::Ptr get(const llvm::Function* function);
    Function::Ptr get(const std::string& fname);

    bool checkVisited(const llvm::Value* val) const;
    const llvm::Module* getInstance() const;
    const GlobalsMap& getGloabls() const;
    void setGlobal(const llvm::Value* val, Domain::Ptr domain);
    Domain::Ptr findGlobal(const llvm::Value* val) const;
    GlobalsMap getGlobalsFor(const Function::Ptr f) const;
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

private:

    void initGlobals(std::vector<const llvm::GlobalVariable*>& globals);

    const llvm::Module* instance_;
    SlotTrackerPass* ST_;
    DomainFactory factory_;
    FunctionMap functions_;
    FunctionMap addressTakenFunctions_;
    GlobalsMap globals_;

};

std::ostream& operator<<(std::ostream& s, const Module& m);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m);

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_MODULE_H
