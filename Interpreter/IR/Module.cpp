//
// Created by abdullin on 3/2/17.
//

#include <andersen/include/Andersen.h>

#include "Module.h"
#include "Util/collections.hpp"

namespace borealis {
namespace absint {

Module::Module(const llvm::Module* module, const Andersen* aa) : instance_(module), factory_(aa) {
    // adding global variables
    for (auto&& it : instance_->getGlobalList()) {
        auto globalDomain = factory_.getBottom(*it.getType());
        if (globalDomain) globals_.insert( {&it, globalDomain} );
    }
}


Function::Ptr Module::createFunction(const llvm::Function* function) {
    if (auto&& opt = util::at(functions_, function)) {
        return opt.getUnsafe();
    } else {
        auto result = Function::Ptr{ new Function(function, &factory_) };
        functions_.insert( {function, result} );
        return result;
    }
}

Function::Ptr Module::createFunction(const std::string& fname) {
    auto&& function = instance_->getFunction(fname);
    return (function) ? createFunction(function) : nullptr;
}

Function::Ptr Module::contains(const llvm::Function* function) const {
    if (auto&& opt = util::at(functions_, function))
        return opt.getUnsafe();
    return nullptr;
}

Module::GlobalsMap& Module::getGloabls() {
    return globals_;
}

Domain::Ptr Module::findGLobal(const llvm::Value* val) const {
    auto&& it = globals_.find(val);
    return (it == globals_.end()) ? nullptr : it->second;
}

void Module::setGlobal(const llvm::Value* val, Domain::Ptr domain) {
    globals_[val] = domain;
}

std::string Module::toString() const {
    std::ostringstream ss;

    if (not globals_.empty()) {
        ss << "globals: " << std::endl;
        for (auto&& global : globals_) {
            ss << " ";
            ss << global.first->getName().str() << " = ";
            ss << global.second->toString() << std::endl;
        }
    }
    ss << std::endl;
    for (auto&& it : functions_) {
        ss << std::endl << it.second << std::endl;
    }
    return ss.str();
}

DomainFactory* Module::getDomainFactory() {
    return &factory_;
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
