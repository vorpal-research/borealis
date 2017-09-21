//
// Created by abdullin on 2/7/17.
//

#include "State.h"
#include "Util/collections.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

State::State(SlotTracker* tracker) : retval_(nullptr), tracker_(tracker) {}
State::State(const State &other) : locals_(other.locals_),
                                   retval_(other.retval_),
                                   tracker_(other.tracker_) {}

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
        locals_[it.first] = find(it.first) ?
                            locals_[it.first]->join(it.second) :
                            it.second;
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

std::string State::toString() const {
    std::ostringstream ss;

    if (not locals_.empty()) {
        for (auto&& local : locals_) {
            ss << "    ";
            ss << tracker_->getLocalName(local.first) << " = ";
            ss << local.second->toString() << std::endl;
        }
    }
    return ss.str();
}

bool State::equals(const State* other) const {
    return this->retval_ == other->retval_ &&
            util::equal_with_find(this->locals_, other->locals_, LAM(a, a.first),
                                  LAM2(a, b, a.second->equals(b.second.get())));
}

bool operator==(const State& lhv, const State& rhv) {
    return lhv.equals(&rhv);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"