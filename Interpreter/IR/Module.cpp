//
// Created by abdullin on 3/2/17.
//

#include <andersen/include/Andersen.h>

#include "Interpreter/Util.h"
#include "Module.h"
#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Module::Module(const llvm::Module* module, SlotTrackerPass* st)
        : instance_(module),
          ST_(st),
          factory_(ST_) {
    /// Initialize all global variables
    initGLobals();
}

void Module::initGLobals() {
    for (auto&& it : instance_->getGlobalList()) {
        Domain::Ptr globalDomain;
        if (it.hasInitializer()) {
            Domain::Ptr content;
            auto& elementType = *it.getType()->getPointerElementType();
            // If global is simple type, that we should wrap it like array of size 1
            if (elementType.isIntegerTy() || elementType.isFloatingPointTy()) {
                auto simpleConst = factory_.get(it.getInitializer());
                auto&& arrayType = llvm::ArrayType::get(&elementType, 1);
                content = factory_.getAggregateObject(*arrayType, {simpleConst});
            // else just create domain
            } else {
                content = factory_.get(it.getInitializer());
            }
            ASSERT(content, "Unsupported constant");

            PointerLocation loc = {factory_.getIndex(0), content};
            auto newDomain = factory_.getPointer(elementType, {loc});
            // we need this because GEPs for global structs and arrays contain one additional index at the start
            if (not (elementType.isIntegerTy() || elementType.isFloatingPointTy())) {
                auto newArray = llvm::ArrayType::get(it.getType(), 1);
                auto newLevel = factory_.getAggregateObject(*newArray, {newDomain});
                PointerLocation loc2 = {factory_.getIndex(0), newLevel};
                globalDomain = factory_.getPointer(*newArray, {loc2});
            } else {
                globalDomain = newDomain;
            }

        } else {
            factory_.getBottom(*it.getType());
        }

        if (not globalDomain) continue;
        globals_.insert( {&it, globalDomain} );
    }
}

Function::Ptr Module::create(const llvm::Function* function) {
    if (auto&& opt = util::at(functions_, function)) {
        return opt.getUnsafe();
    } else {
        auto result = Function::Ptr{ new Function(function, &factory_, ST_->getSlotTracker(function)) };
        functions_.insert( {function, result} );
        return result;
    }
}

Function::Ptr Module::create(const std::string& fname) {
    auto&& function = instance_->getFunction(fname);
    return (function) ? create(function) : nullptr;
}

Function::Ptr Module::get(const llvm::Function* function) const {
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
            ss << "  ";
            ss << global.first->getName().str() << " = ";
            ss << global.second->toString("  ") << std::endl;
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

const Module::FunctionMap& Module::getFunctions() const {
    return functions_;
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

#include "Util/unmacros.h"
