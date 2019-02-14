//
// Created by abdullin on 10/26/17.
//

#include <llvm/IR/Constants.h>

#include "GlobalVariableManager.h"
#include "Interpreter/Domain/Domain.h"
#include "Interpreter/Domain/DomainFactory.h"
#include "Interpreter/IR/Module.h"
#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ir {

//GlobalVar::GlobalVar(const llvm::GlobalVariable* value)
//        : value_(value), color_(WHITE) {
//    initEdges();
//}
//
//const GlobalVar::Edges& GlobalVar::getEdges() const {
//    return edges_;
//}
//
//const llvm::GlobalVariable* GlobalVar::getValue() {
//    return value_;
//}
//
//GlobalVar::Color GlobalVar::getColor() const {
//    return color_;
//}
//
//void GlobalVar::setColor(GlobalVar::Color color) {
//    color_ = color;
//}
//
//void GlobalVar::addEdge(const llvm::GlobalVariable* edge) {
//    edges_.insert(edge);
//}
//
//void GlobalVar::initEdges() {
//    if (not value_->hasInitializer()) return;
//    for (auto&& op : value_->getInitializer()->operands()) {
//        getAllGlobals(edges_, op);
//    }
//}
//
//void GlobalVar::getAllGlobals(std::unordered_set<const llvm::GlobalVariable*>& globals, const llvm::Value* value) {
//    if (auto global = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
//        globals.insert(global);
//    } else if (auto constantExpr = llvm::dyn_cast<llvm::ConstantExpr>(value)) {
//        for (auto&& op : constantExpr->operands()) {
//            getAllGlobals(globals, op);
//        }
//    } else if (auto constantArray = llvm::dyn_cast<llvm::ConstantArray>(value)) {
//        for (auto i = 0U; i < constantArray->getNumOperands(); ++i) {
//            getAllGlobals(globals, constantArray->getOperand(i));
//        }
//    } else if (auto&& constantStruct = llvm::dyn_cast<llvm::ConstantStruct>(value)) {
//        for (auto i = 0U; i < constantStruct->getNumOperands(); ++i) {
//            getAllGlobals(globals, constantStruct->getOperand(i));
//        }
//    }
//}
//
//GlobalVariableManager::GlobalVariableManager(Module* module) : module_(module), DF_(module_->getDomainFactory()) {}
//
//void GlobalVariableManager::init(const std::vector<const llvm::GlobalVariable*>& globs) {
//    // find all global variables
//    std::unordered_set<const llvm::GlobalVariable*> all_globals;
//    for (auto&& it : globs) {
//        all_globals.insert(it);
//        if (it->hasInitializer()) GlobalVar::getAllGlobals(all_globals, it->getInitializer());
//    }
//
//    // do topological sort of all global variables, find the right ordering for their initialization
//    std::vector<const llvm::GlobalVariable*> ordered;
//    std::unordered_set<const llvm::GlobalVariable*> cycled;
//    std::unordered_map<const llvm::GlobalVariable*, GlobalVar::Ptr> globalsMap;
//    for (auto&& it : all_globals) {
//        auto global = std::make_shared<GlobalVar>(it);
//        if (global->getEdges().empty()) ordered.emplace_back(it);
//        else globalsMap[it] = global;
//    }
//    for (auto&& it : globalsMap) {
//        if (it.second->getColor() == GlobalVar::WHITE) {
//            topologicalSort(it.first, globalsMap, ordered, cycled);
//        }
//    }
//    // preinit cycled globals
//    for (auto&& it : cycled) {
//        globals_[it] = DF_->getBottom(DF_->cast(it->getType()));
//    }
//    // init all other globals
//    for (auto&& it : ordered) {
//        globals_.insert( {it, DF_->get(it)} );
//        names_.insert({it->getName().str(), it});
//    }
//}
//
//void GlobalVariableManager::topologicalSort(const llvm::GlobalVariable* current,
//                                     std::unordered_map<const llvm::GlobalVariable*, GlobalVar::Ptr>& globals,
//                                     std::vector<const llvm::GlobalVariable*>& ordered,
//                                     std::unordered_set<const llvm::GlobalVariable*>& cycled) {
//    if (not util::at(globals, current)) return;
//    if (globals[current]->getColor() == GlobalVar::BLACK) return;
//    if (globals[current]->getColor() == GlobalVar::GREY) {
//        cycled.insert(current);
//        return;
//    }
//    globals[current]->setColor(GlobalVar::GREY);
//    for (auto&& edge : globals[current]->getEdges()) {
//        topologicalSort(edge, globals, ordered, cycled);
//    }
//    globals[current]->setColor(GlobalVar::BLACK);
//    ordered.emplace_back(current);
//}
//
//Domain::Ptr GlobalVariableManager::findGlobal(const llvm::Value* val) const {
//    auto&& it = globals_.find(val);
//    return (it == globals_.end()) ? nullptr : it->second;
//}
//
//Domain::Ptr GlobalVariableManager::get(const std::string& name) {
//    if (auto&& opt = util::at(names_, name)) {
//        auto domain = findGlobal(opt.getUnsafe());
//        return domain;
//
//    } else if (auto func = module_->get(name)) {
//        auto funcDomain = DF_->get(llvm::cast<llvm::Constant>(func->getInstance()));
//        globals_[func->getInstance()] = funcDomain;
//        return funcDomain;
//    }
//    UNREACHABLE("Unknown global: " + name);
//}
//
//Function::Ptr GlobalVariableManager::get(const llvm::Function* function) {
//    return module_->get(function);
//}
//
//const GlobalVariableManager::DomainMap& GlobalVariableManager::getGlobals() const {
//    return globals_;
//}
//
//Function::Ptr GlobalVariableManager::getFunctionByName(const std::string& name) {
//    return module_->get(name);
//}

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"