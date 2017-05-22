//
// Created by abdullin on 3/2/17.
//

#ifndef BOREALIS_MODULE_H
#define BOREALIS_MODULE_H

#include <string>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>

#include "Function.h"
#include "Interpreter/Domain/DomainFactory.h"

namespace borealis {
namespace absint {

class Module {
public:

    using GlobalsMap = std::map<const llvm::Value*, Domain::Ptr>;
    using FunctionMap = std::map<const llvm::Function*, Function::Ptr>;

    Module(const llvm::Module* module);
    Function::Ptr contains(const llvm::Function* function) const;
    Function::Ptr createFunction(const llvm::Function* function);
    Function::Ptr createFunction(const std::string& fname);

    GlobalsMap& getGloabls();
    void setGlobal(const llvm::Value* val, Domain::Ptr domain);
    Domain::Ptr findGLobal(const llvm::Value* val) const;
    const FunctionMap& getFunctions() const;

    DomainFactory* getDomainFactory();

    std::string toString() const;


private:

    void initGLobals();

    const llvm::Module* instance_;
    DomainFactory factory_;
    FunctionMap functions_;
    GlobalsMap globals_;

};

std::ostream& operator<<(std::ostream& s, const Module& m);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m);

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_MODULE_H
