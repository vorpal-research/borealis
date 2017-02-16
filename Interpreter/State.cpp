//
// Created by abdullin on 2/7/17.
//

#include "State.h"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

bool compare(const State::Map& lhv, const State::Map& rhv) {
    if (lhv.size() != rhv.size()) return false;
    for (auto&& it : lhv) {
        auto&& its = rhv.find(it.first);
        if (its == rhv.end()) return false;
        if (not it.second->equals(its->second.get())) return false;
    }
    return true;
}

State::State(Environment::Ptr environment) : environment_(environment), retval_(nullptr) {}
State::State(const State &other) : environment_(other.environment_), globals_(other.globals_),
                                   locals_(other.locals_), retval_(other.retval_) {}

void State::addGlobalVariable(const llvm::Value *val, Domain::Ptr domain) {
    globals_[val] = domain;
}

void State::addLocalVariable(const llvm::Value *val, Domain::Ptr domain) {
    locals_[val] = domain;
}

void State::setReturnValue(Domain::Ptr domain) {
    ASSERT(not retval_, "Reassigning retval in state");
    retval_ = domain;
}

void State::mergeToReturnValue(Domain::Ptr domain) {
    retval_ = (not retval_) ? domain : retval_->join(domain);
}

Environment::Ptr State::getEnvironment() const { return environment_; }
const State::Map& State::getGlobals() const { return globals_; }
const State::Map& State::getLocals() const { return locals_; }
Domain::Ptr State::getReturnValue() const { return retval_; }


void State::merge(State::Ptr other) {
    mergeGlobal(other);
    mergeLocal(other);
    mergeReturnValue(other);
}

void State::mergeGlobal(State::Ptr other) {
    for (auto&& it : other->globals_) {
        if (findGlobal(it.first)) {
            globals_[it.first] = globals_[it.first]->join(it.second);
        } else {
            globals_[it.first] = it.second;
        }
    }
}

void State::mergeLocal(State::Ptr other) {
    for (auto&& it : other->locals_) {
        if (findLocal(it.first)) {
            locals_[it.first] = locals_[it.first]->join(it.second);
        } else {
            locals_[it.first] = it.second;
        }
    }
}

void State::mergeReturnValue(State::Ptr other) {
    if (not other->retval_) return;
    if (not retval_) retval_ = other->retval_;
    else retval_ = retval_->join(other->retval_);
}

Domain::Ptr State::find(const llvm::Value *val) const {
    if (auto global = findGlobal(val)) return global;
    if (auto local = findLocal(val)) return local;
    return nullptr;
}

Domain::Ptr State::findGlobal(const llvm::Value *val) const {
    auto&& it = globals_.find(val);
    return (it == globals_.end()) ? nullptr : it->second;
}

Domain::Ptr State::findLocal(const llvm::Value *val) const {
    auto&& it = locals_.find(val);
    return (it == locals_.end()) ? nullptr : it->second;
}

bool State::empty() const {
    return locals_.empty() && globals_.empty();
}

std::string State::toString() const {
    std::ostringstream ss;
    auto& tracker = environment_->getSlotTracker();

    if (not globals_.empty()) {
        ss << "  globals: " << std::endl;
        for (auto&& global : globals_) {
            ss << "    " << tracker.getLocalName(global.first) << " = " << global.second->toString() << std::endl;
        }
    }
    if (not locals_.empty()) {
        ss << "  locals: " << std::endl;
        for (auto&& local : locals_) {
            ss << "    " << tracker.getLocalName(local.first) << " = " << local.second->toString() << std::endl;
        }
    }
    return ss.str();
}

bool State::equals(const State* other) const {
    return compare(this->globals_, other->globals_) &&
           compare(this->locals_, other->locals_) &&
           this->retval_ == other->retval_;
}

bool operator==(const State& lhv, const State& rhv) {
    return lhv.equals(&rhv);
}
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"