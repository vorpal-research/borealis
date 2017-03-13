//
// Created by abdullin on 2/8/17.
//

#include "Environment.h"
#include "Interpreter/IR/Function.h"

namespace borealis {
namespace absint {

Environment::Environment(const llvm::Module* module) : module_(module) {}

DomainFactory& Environment::getDomainFactory() const {
    return factory_;
}

const llvm::Module* Environment::getModule() const {
    return module_;
}

}   /* namespace absint */
}   /* namespace borealis */

