//
// Created by abdullin on 2/8/17.
//

#include "Environment.h"
#include "Interpreter/IR/Function.h"

namespace borealis {
namespace absint {

Environment::Environment(const llvm::Module* module) : module_(module) {}

const DomainFactory& Environment::getDomainFactory() const {
    return factory_;
}

const llvm::Module* Environment::getModule() const {
    return module_;
}

Function::Ptr Environment::getFunction(const llvm::Function* function) const {
    return Function::Ptr{ new Function(shared_from_this(), function) };
}

}   /* namespace absint */
}   /* namespace borealis */

