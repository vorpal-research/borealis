//
// Created by abdullin on 2/10/17.
//

#ifndef BOREALIS_MODULE_H
#define BOREALIS_MODULE_H

#include "Interpreter/Environment.h"
#include "Function.h"

namespace borealis {
namespace absint {

class Module {
public:
    using FunctionMap = std::unordered_map<const llvm::Function*, Function>;

    Module(Environment::Ptr environment, const llvm::Module* module);

    Environment::Ptr getEnvironment() const;
    const llvm::Module* getInstance() const;
    const FunctionMap& getFunctions() const;

    const Function* getFunction(const llvm::Function* function) const;
    const Function* getFunction(const llvm::Function& function) const {
        return getFunction(&function);
    }
    const Function* getFunction(const std::string& fname) const;

    std::string toString() const;

private:

    Environment::Ptr environment_;
    const llvm::Module* instance_;
    FunctionMap functions_;
};

std::ostream& operator<<(std::ostream& s, const Module& m);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m);

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_MODULE_H
