//
// Created by abdullin on 3/2/17.
//

#include "Interpreter/Util.hpp"
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
    initGlobals();
}

void Module::initGlobals() {
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
            globalDomain = factory_.getBottom(*it.getType());
        }

        if (not globalDomain) {
            errs() << "Could not create domain for: " << ST_->toString(&it) << endl;
            continue;
        }
        globals_.insert( {&it, globalDomain} );
    }
}

Function::Ptr Module::get(const llvm::Function* function) {
    if (auto&& opt = util::at(functions_, function)) {
        return opt.getUnsafe();
    } else {
        auto result = Function::Ptr{ new Function(function, &factory_, ST_->getSlotTracker(function)) };
        functions_.insert( {function, result} );
        return result;
    }
}

Function::Ptr Module::get(const std::string& fname) {
    auto&& function = instance_->getFunction(fname);
    return (function) ? get(function) : nullptr;
}

const llvm::Module* Module::getInstance() const {
    return instance_;
}

const Module::GlobalsMap& Module::getGloabls() const {
    return globals_;
}

Domain::Ptr Module::findGlobal(const llvm::Value* val) const {
    auto&& it = globals_.find(val);
    return (it == globals_.end()) ? nullptr : it->second;
}

void Module::setGlobal(const llvm::Value* val, Domain::Ptr domain) {
    globals_[val] = domain;
}

std::string Module::toString() const {
    std::ostringstream ss;
    ss << *this;
    return ss.str();
}

DomainFactory* Module::getDomainFactory() {
    return &factory_;
}

SlotTrackerPass* Module::getSlotTracker() const {
    return ST_;
}

const Module::FunctionMap& Module::getFunctions() const {
    return functions_;
}

Domain::Ptr Module::getDomainFor(const llvm::Value* value, const llvm::BasicBlock* location) {
    if (llvm::isa<llvm::GlobalVariable>(value)) {
        return findGlobal(value);
    } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(value)) {
        return factory_.get(constant);
    } else {
        return get(location->getParent())->getDomainFor(value, location);
    }
    UNREACHABLE("Unknown type of value: " + ST_->toString(value));
}

Module::GlobalsMap Module::getGlobalsFor(const Function::Ptr f) const {
    return util::viewContainer(f->getGlobals())
            .map([&](auto&& a) -> std::pair<const llvm::Value*, Domain::Ptr> { return {a, findGlobal(a)}; })
            .toMap();
}

Module::GlobalsMap Module::getGlobalsFor(const BasicBlock* bb) const {
    return util::viewContainer(bb->getGlobals())
            .map([&](auto&& a) -> std::pair<const llvm::Value*, Domain::Ptr> { return {a, findGlobal(a)}; })
            .toMap();
}

std::ostream& operator<<(std::ostream& s, const Module& m) {
    if (not m.getGloabls().empty()) {
        s << "Global Variables: " << std::endl;
        s.flush();
        for (auto&& global : m.getGloabls()) {
            s << "  " << global.first->getName().str() << " = " << global.second->toPrettyString("  ") << std::endl;
            s.flush();
        }
    }
    s << std::endl;
    s.flush();
    for (auto&& it : m.getFunctions()) {
        s << std::endl << std::endl << it.second << std::endl << std::endl;
        s.flush();
    }
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m) {
    if (not m.getGloabls().empty()) {
        s << "Global Variables: " << endl;
        s.flush();
        for (auto&& global : m.getGloabls()) {
            s << "  " << global.first->getName().str() << " = " << global.second->toPrettyString("  ") << endl;
            s.flush();
        }
    }
    s << endl;
    s.flush();
    for (auto&& it : m.getFunctions()) {
        s << endl << endl << it.second << endl << endl;
        s.flush();
    }
    return s;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
