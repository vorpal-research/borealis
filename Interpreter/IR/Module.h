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
    /// Returns the function, if it already was created
    Function::Ptr get(const llvm::Function* function) const;
    /// Creates the function and saves it
    Function::Ptr create(const llvm::Function* function);
    Function::Ptr create(const std::string& fname);

    GlobalsMap& getGloabls();
    void setGlobal(const llvm::Value* val, Domain::Ptr domain);
    Domain::Ptr findGlobal(const llvm::Value* val) const;
    const FunctionMap& getFunctions() const;

    DomainFactory* getDomainFactory();

    std::string toString() const;


private:

    void initGlobals();

    const llvm::Module* instance_;
    SlotTrackerPass* ST_;
    DomainFactory factory_;
    FunctionMap functions_;
    GlobalsMap globals_;

};

std::ostream& operator<<(std::ostream& s, const Module& m);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m);

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_MODULE_H
