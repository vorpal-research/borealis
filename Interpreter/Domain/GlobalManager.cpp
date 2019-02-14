//
// Created by abdullin on 2/11/19.
//

#include <llvm/IR/Constants.h>

#include "GlobalManager.hpp"
#include "Interpreter/IR/Module.h"
#include "VariableFactory.hpp"
#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

namespace impl_ {

GlobalVar::GlobalVar(const llvm::GlobalVariable* value)
        : value_(value), color_(WHITE) {
    initEdges();
}

const GlobalVar::Edges& GlobalVar::getEdges() const {
    return edges_;
}

const llvm::GlobalVariable* GlobalVar::getValue() {
    return value_;
}

GlobalVar::Color GlobalVar::getColor() const {
    return color_;
}

void GlobalVar::setColor(GlobalVar::Color color) {
    color_ = color;
}

void GlobalVar::addEdge(const llvm::GlobalVariable* edge) {
    edges_.insert(edge);
}

void GlobalVar::initEdges() {
    if (not value_->hasInitializer()) return;
    for (auto&& op : value_->getInitializer()->operands()) {
        getAllGlobals(edges_, op);
    }
}

void GlobalVar::getAllGlobals(std::unordered_set<const llvm::GlobalVariable*>& globals, const llvm::Value* value) {
    if (auto global = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
        globals.insert(global);
    } else if (auto constantExpr = llvm::dyn_cast<llvm::ConstantExpr>(value)) {
        for (auto&& op : constantExpr->operands()) {
            getAllGlobals(globals, op);
        }
    } else if (auto constantArray = llvm::dyn_cast<llvm::ConstantArray>(value)) {
        for (auto i = 0U; i < constantArray->getNumOperands(); ++i) {
            getAllGlobals(globals, constantArray->getOperand(i));
        }
    } else if (auto&& constantStruct = llvm::dyn_cast<llvm::ConstantStruct>(value)) {
        for (auto i = 0U; i < constantStruct->getNumOperands(); ++i) {
            getAllGlobals(globals, constantStruct->getOperand(i));
        }
    }
}

} // namespace impl_

GlobalManager::GlobalManager(ir::Module* module) : module_(module) {}

void GlobalManager::init(const std::vector<const llvm::GlobalVariable*>& globs) {
    // find all global variables
    std::unordered_set<const llvm::GlobalVariable*> all_globals;
    for (auto&& it : globs) {
        all_globals.insert(it);
        if (it->hasInitializer()) impl_::GlobalVar::getAllGlobals(all_globals, it->getInitializer());
    }

    // do topological sort of all global variables, find the right ordering for their initialization
    std::vector<const llvm::GlobalVariable*> ordered;
    std::unordered_set<const llvm::GlobalVariable*> cycled;
    std::unordered_map<const llvm::GlobalVariable*, impl_::GlobalVar::Ptr> globalsMap;
    for (auto&& it : all_globals) {
        auto global = std::make_shared<impl_::GlobalVar>(it);
        if (global->getEdges().empty()) ordered.emplace_back(it);
        else globalsMap[it] = global;
    }
    for (auto&& it : globalsMap) {
        if (it.second->getColor() == impl_::GlobalVar::WHITE) {
            topologicalSort(it.first, globalsMap, ordered, cycled);
        }
    }
    // preinit cycled globals
    for (auto&& it : cycled) {
        globals_[it] = module_->variableFactory()->bottom(it->getType());
    }
    // init all other globals
    for (auto&& it : ordered) {
        globals_.insert( {it, allocate(it)} );
        names_.insert({it->getName().str(), it});
    }
}

void GlobalManager::topologicalSort(const llvm::GlobalVariable* current,
                                    std::unordered_map<const llvm::GlobalVariable*, impl_::GlobalVar::Ptr>& globals,
                                    std::vector<const llvm::GlobalVariable*>& ordered,
                                    std::unordered_set<const llvm::GlobalVariable*>& cycled) {
    if (not util::at(globals, current)) return;
    if (globals[current]->getColor() == impl_::GlobalVar::BLACK) return;
    if (globals[current]->getColor() == impl_::GlobalVar::GREY) {
        cycled.insert(current);
        return;
    }
    globals[current]->setColor(impl_::GlobalVar::GREY);
    for (auto&& edge : globals[current]->getEdges()) {
        topologicalSort(edge, globals, ordered, cycled);
    }
    globals[current]->setColor(impl_::GlobalVar::BLACK);
    ordered.emplace_back(current);
}

AbstractDomain::Ptr GlobalManager::global(const llvm::Value* val) const {
    auto&& it = globals_.find(val);
    return (it == globals_.end()) ? nullptr : it->second;
}

AbstractDomain::Ptr GlobalManager::get(const std::string& name) {
    if (auto&& opt = util::at(names_, name)) {
        auto domain = global(opt.getUnsafe());
        return domain;

    } else if (auto func = module_->get(name)) {
        AbstractDomain::Ptr funcDomain = allocate(func->getInstance());
        globals_[func->getInstance()] = funcDomain;
        return funcDomain;
    }
    UNREACHABLE("Unknown global: " + name);
}

ir::Function::Ptr GlobalManager::get(const llvm::Function* function) {
    return module_->get(function);
}

const GlobalManager::DomainMap& GlobalManager::globals() const {
    return globals_;
}

ir::Function::Ptr GlobalManager::getFunctionByName(const std::string& name) {
    return module_->get(name);
}

AbstractDomain::Ptr GlobalManager::allocate(const llvm::GlobalObject* object) const {
    auto vf_ = module_->variableFactory();
    if (auto* function = llvm::dyn_cast<const llvm::Function>(object)) {
        return vf_->get(llvm::cast<llvm::Function>(function));
    } else if (auto* global = llvm::dyn_cast<const llvm::GlobalVariable>(object)) {
        return vf_->get(global);
    } else {
        UNREACHABLE("Unknown global: " + object->getName().str());
    }
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"