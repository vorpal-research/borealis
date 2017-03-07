//
// Created by abdullin on 3/2/17.
//

#ifndef BOREALIS_MODULE_H
#define BOREALIS_MODULE_H

#include <llvm/IR/Function.h>

#include "Function.h"
#include "Interpreter/Environment.h"

namespace borealis {
namespace absint {

class Module {
protected:

    friend class Interpreter;
    Module() = default;

public:

    using FunctionMap = std::unordered_map<const llvm::Function*, Function::Ptr>;

    Module(Environment::Ptr environment);
    Function::Ptr contains(const llvm::Function* function) const;
    Function::Ptr createFunction(const llvm::Function* function);
    Function::Ptr createFunction(const std::string& fname);

    std::string toString() const;


private:

    Environment::Ptr environment_;
    FunctionMap functions_;

};

std::ostream& operator<<(std::ostream& s, const Module& m);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m);

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_MODULE_H
