//
// Created by abdullin on 2/7/17.
//

#include "State.h"
#include "Util/collections.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

void mergeMaps(State::VariableMap& lhv, State::VariableMap& rhv) {
    if (lhv.sameAs(rhv)) return;

    auto&& lhv_end = lhv.end();
    for (auto&& it : rhv) {
        auto&& lhv_it = lhv.find(it.first);
        lhv[it.first] = (lhv_it == lhv_end) ?
                        it.second :
                        lhv_it->second->join(it.second);
    }
}

State::State(SlotTracker* tracker) :
        retval_(nullptr),
        tracker_(tracker) {}
State::State(const State &other) : locals_(other.locals_),
                                   retval_(other.retval_),
                                   tracker_(other.tracker_) {}

void State::addVariable(const llvm::Value* val, Domain::Ptr domain) {
    auto inst = llvm::dyn_cast<llvm::Instruction>(val);
    if (inst) {
        addVariable(inst, domain);
    } else {
        other_locals_[val] = domain;
    }
}

void State::addVariable(const llvm::Instruction* inst, Domain::Ptr domain) {
    auto bb = inst->getParent();
    locals_[bb][inst] = domain;
}

void State::setReturnValue(Domain::Ptr domain) {
    ASSERT(not retval_, "Reassigning retval in state");
    retval_ = domain;
}

void State::mergeToReturnValue(Domain::Ptr domain) {
    retval_ = (not retval_) ? domain : retval_->join(domain);
}

const State::BlockMap& State::getLocals() const { return locals_; }
Domain::Ptr State::getReturnValue() const { return retval_; }


void State::merge(State::Ptr other) {
    mergeVariables(other);
    mergeReturnValue(other);
}

void State::mergeVariables(State::Ptr other) {
    mergeMaps(other_locals_, other->other_locals_);
    for (auto&& it : other->locals_) {
        if (auto&& its = util::at(locals_, it.first)) {
            mergeMaps(its.getUnsafe(), it.second);
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
    auto inst = llvm::dyn_cast<llvm::Instruction>(val);
    if (inst) {
        if (auto&& bbMap = util::at(locals_, inst->getParent())) {
            auto&& it = bbMap.getUnsafe().find(val);
            return (it == bbMap.getUnsafe().end()) ? nullptr : it->second;
        }
    } else {
        auto&& it = other_locals_.find(val);
        return (it == other_locals_.end()) ? nullptr : it->second;
    }
    return nullptr;
}

bool State::empty() const {
    return locals_.empty();
}

std::string State::toString() const {
    std::ostringstream ss;

    if (not locals_.empty()) {
        for (auto&& bb : locals_) {
            ss << bb.first->getName().str() << std::endl;
            for (auto&& local : bb.second) {
                ss << "    ";
                ss << tracker_->getLocalName(local.first) << " = ";
                ss << local.second->toString() << std::endl;
            }
        }
    }
    return ss.str();
}

bool State::equals(const State* other) const {
    auto&& equal = [](const State::VariableMap& lhv, const State::VariableMap& rhv) {
        if (lhv.sameAs(rhv)) return true;
        return util::equal_with_find(lhv, rhv, LAM(a, a.first), LAM2(a, b, a.second->equals(b.second.get())));
    };

    if (this->retval_ != other->retval_) {
        return false;
    }

    if (this->locals_.size() != other->locals_.size()) {
        return false;
    }

    if (not equal(other_locals_, other->other_locals_)) {
        return false;
    }

    for (auto&& its : other->locals_) {
        if (auto&& it = util::at(locals_, its.first)) {
            if (not equal(it.getUnsafe(), its.second)) {
                return false;
            }

        } else {
            return false;
        }
    }

    return true;
}

bool operator==(const State& lhv, const State& rhv) {
    return lhv.equals(&rhv);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"