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

State::State() : retval_(nullptr) {}
State::State(const State &other) : locals_(other.locals_),
                                   retval_(other.retval_) {}

void State::addVariable(const llvm::Value* val, Domain::Ptr domain) {
    locals_[val] = domain;
}

void State::setReturnValue(Domain::Ptr domain) {
    ASSERT(not retval_, "Reassigning retval in state");
    retval_ = domain;
}

void State::mergeToReturnValue(Domain::Ptr domain) {
    retval_ = (not retval_) ? domain : retval_->join(domain);
}

const State::Map& State::getLocals() const { return locals_; }
Domain::Ptr State::getReturnValue() const { return retval_; }


void State::merge(State::Ptr other) {
    mergeVariables(other);
    mergeReturnValue(other);
}

void State::mergeVariables(State::Ptr other) {
    for (auto&& it : other->locals_) {
        if (find(it.first)) {
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
    auto&& it = locals_.find(val);
    return (it == locals_.end()) ? nullptr : it->second;
}

bool State::empty() const {
    return locals_.empty();
}

std::string State::toString(SlotTracker& tracker) const {
    std::ostringstream ss;

    if (not locals_.empty()) {
        ss << "  locals: " << std::endl;
        for (auto&& local : locals_) {
            ss << "    ";
            ss << tracker.getLocalName(local.first) << " = ";
            ss << local.second->toString() << std::endl;
        }
    }
    return ss.str();
}

bool State::equals(const State* other) const {
    return compare(this->locals_, other->locals_) &&
           this->retval_ == other->retval_;
}

bool operator==(const State& lhv, const State& rhv) {
    return lhv.equals(&rhv);
}
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"