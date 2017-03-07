//
// Created by abdullin on 3/2/17.
//

#include "Module.h"
#include "Util/collections.hpp"

namespace borealis {
namespace absint {

Module::Module(borealis::absint::Environment::Ptr environment) : environment_(environment) {}

Function::Ptr Module::createFunction(const llvm::Function* function) {
    if (auto&& opt = util::at(functions_, function)) {
        return opt.getUnsafe();
    } else {
        auto result = Function::Ptr{ new Function(environment_, function) };
        functions_.insert( {function, result} );
        return result;
    }
}

Function::Ptr Module::createFunction(const std::string& fname) {
    auto&& function = environment_->getModule()->getFunction(fname);
    return (function) ? createFunction(function) : nullptr;
}

Function::Ptr Module::contains(const llvm::Function* function) const {
    if (auto&& opt = util::at(functions_, function))
        return opt.getUnsafe();
    return nullptr;
}

std::string Module::toString() const {
    std::ostringstream ss;
    for (auto&& it : functions_) {
        ss << std::endl << it.second << std::endl;
    }
    return ss.str();
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
