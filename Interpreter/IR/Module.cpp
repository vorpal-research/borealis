//
// Created by abdullin on 2/10/17.
//

#include "Module.h"
#include "Util/collections.hpp"

namespace borealis {
namespace absint {

Module::Module(Environment::Ptr environment, const llvm::Module* module) : environment_(environment),
                                                                           instance_(module) {
    for (auto&& function : util::viewContainer(*instance_)) {
        if (not function.isDeclaration()) {
            functions_.insert({&function, Function(environment_, &function)});
        }
    }
    // TODO: add global variables
}

Environment::Ptr Module::getEnvironment() const {
    return environment_;
}

const llvm::Module* Module::getInstance() const {
    return instance_;
}

const Module::FunctionMap& Module::getFunctions() const {
    return functions_;
}

std::string Module::toString() const {
    std::ostringstream ss;

    ss << "Module ";
    ss << instance_->getModuleIdentifier() << std::endl;

    for (auto&& it : util::viewContainer(functions_)) {
        ss << std::endl << it.second.toString() << std::endl;
    }
    return ss.str();
}

const Function* Module::getFunction(const llvm::Function* function) const {
    if (auto&& opt = util::at(functions_, function))
        return &opt.getUnsafe();
    return nullptr;
}

const Function* Module::getFunction(const std::string& fname) const {
    for (auto&& it : util::viewContainer(functions_)) {
        if (it.second.getName() == fname) return &(it.second);
    }
    return nullptr;
}

std::ostream& operator<<(std::ostream& s, const Module& m) {
    s << m.toString();
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m) {
    s << m.toString();
    return s;
}

}   /* namespace absint */
}   /* namespace borealis */