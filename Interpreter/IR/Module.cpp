//
// Created by abdullin on 3/2/17.
//

#include "Config/config.h"
#include "Interpreter/Util.hpp"
#include "Module.h"
#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ir {

static config::MultiConfigEntry roots("analysis", "root-function");

Module::Module(const llvm::Module* module, SlotTrackerPass* st, bool initAddrTakenFuncs)
        : instance_(module),
          ST_(st),
          gm_(this),
          vf_(instance_->getDataLayout(), &gm_),
          GVM_(this),
          factory_(ST_, &GVM_, instance_->getDataLayout()) {
    if (initAddrTakenFuncs) {
        initAddressTakenFunctions();
    }
}

void Module::initAddressTakenFunctions() {
    for (auto&& it : instance_->getFunctionList())
        if (llvm::hasAddressTaken(it)) addressTakenFunctions_.insert({&it, get(&it)});
}

void Module::initGlobals(const std::unordered_set<const llvm::Value*>& globals) {
    GVM_.init(util::viewContainer(globals).map(llvm::dyn_caster<llvm::GlobalVariable>()).toVector());
}

std::vector<Function::Ptr> Module::getRootFunctions() {
    return util::viewContainer(roots)
            .map(LAM(name, get(name)))
            .filter()
            .toVector();
}

Function::Ptr Module::get(const llvm::Function* function) {
    if (auto&& opt = util::at(functions_, function)) {
        return opt.getUnsafe();
    } else {
        auto result = std::make_shared<Function>(function, &factory_, ST_->getSlotTracker(function));
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

std::string Module::toString() const {
    std::ostringstream ss;
    ss << *this;
    return ss.str();
}

DomainFactory* Module::getDomainFactory() {
    return &factory_;
}

VariableFactory* Module::getVariableFactory() {
    return &vf_;
}

SlotTrackerPass* Module::getSlotTracker() const {
    return ST_;
}

const Module::FunctionMap& Module::getFunctions() const {
    return functions_;
}

Domain::Ptr Module::getDomainFor(const llvm::Value* value, const llvm::BasicBlock* location) {
    if (llvm::isa<llvm::GlobalVariable>(value)) {
        return GVM_.findGlobal(value);
    } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(value)) {
        return factory_.get(constant);
    } else {
        return get(location->getParent())->getDomainFor(value, location);
    }
}

Module::GlobalsMap Module::getGlobalsFor(Function::Ptr f) const {
    return util::viewContainer(f->getGlobals())
            .map([&](auto&& a) -> std::pair<const llvm::Value*, Domain::Ptr> { return {a, GVM_.findGlobal(a)}; })
            .toMap();
}

Module::GlobalsMap Module::getGlobalsFor(const BasicBlock* bb) const {
    return util::viewContainer(bb->getGlobals())
            .map([&](auto&& a) -> std::pair<const llvm::Value*, Domain::Ptr> { return {a, GVM_.findGlobal(a)}; })
            .toMap();
}

bool function_types_eq(const llvm::Type* lhv, const llvm::Type* rhv) {
    auto* lhvf = llvm::cast<llvm::FunctionType>(lhv);
    auto* rhvf = llvm::cast<llvm::FunctionType>(rhv);
    ASSERT(lhvf && rhvf, "Non-function types in comparing prototypes");
    if (not util::llvm_types_eq(lhvf->getReturnType(), rhvf->getReturnType())) return false;
    if (lhvf->isVarArg() && lhvf->getNumParams() > rhvf->getNumParams()) return false;
    if (not lhvf->isVarArg() && lhvf->getNumParams() != rhvf->getNumParams()) return false;
    for (auto i = 0U; i < lhvf->getNumParams(); ++i) {
        if (not util::llvm_types_eq(lhvf->getParamType(i), rhvf->getParamType(i))) return false;
    }
    return true;
}

std::vector<Function::Ptr> Module::findFunctionsByPrototype(const llvm::Type* prototype) const {
    return util::viewContainer(addressTakenFunctions_)
            .filter(LAM(a, function_types_eq(a.first->getType()->getPointerElementType(), prototype)))
            .map(LAM(a, a.second))
            .toVector();
}

bool Module::checkVisited(const llvm::Value* val) const {
    if (llvm::isa<llvm::Constant>(val)) return true;
    else if (llvm::isa<llvm::GlobalVariable>(val)) return true;
    else if (llvm::isa<llvm::Argument>(val)) return true;
    else if (auto&& inst = llvm::dyn_cast<llvm::Instruction>(val)) {
        auto bb = inst->getParent();
        auto func = bb->getParent();
        auto&& it = functions_.find(func);
        if (it == functions_.end()) return false;
        if (not it->second->isVisited()) return false;
        return it->second->getBasicBlock(bb)->isVisited();
    }
    UNREACHABLE("Unknown value: " + ST_->toString(val));
}

const Module::FunctionMap& Module::getAddressTakenFunctions() const {
    return addressTakenFunctions_;
}

Domain::Ptr Module::findGlobal(const llvm::Value* value) const {
    return GVM_.findGlobal(value);
}

GlobalVariableManager* Module::getGlobalVariableManager() {
    return &GVM_;
}

const GlobalVariableManager* Module::getGlobalVariableManager() const {
    return &GVM_;
}

std::ostream& operator<<(std::ostream& s, const Module& m) {
    if (not m.getGlobalVariableManager()->getGlobals().empty()) {
        s << "Global Variables: " << std::endl;
        for (auto&& global : m.getGlobalVariableManager()->getGlobals()) {
            s << "  " << global.first->getName().str() << " = " << global.second->toPrettyString("  ") << std::endl;
            s.flush();
        }
    }
    s << std::endl;
    for (auto&& it : m.getFunctions()) {
        s << std::endl << std::endl << it.second << std::endl << std::endl;
        s.flush();
    }
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Module& m) {
    if (not m.getGlobalVariableManager()->getGlobals().empty()) {
        s << "Global Variables: " << endl;
        for (auto&& global : m.getGlobalVariableManager()->getGlobals()) {
            s << "  " << global.first->getName().str() << " = " << global.second->toPrettyString("  ") << endl;
            s.flush();
        }
    }
    s << endl;
    for (auto&& it : m.getFunctions()) {
        s << endl << endl << it.second << endl << endl;
        s.flush();
    }
    return s;
}

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
