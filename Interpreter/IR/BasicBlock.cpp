//
// Created by abdullin on 2/9/17.
//

#include "BasicBlock.h"
#include "DomainStorage.hpp"
#include "Interpreter/Domain/VariableFactory.hpp"

#include "Util/collections.hpp"
#include "Util/util.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ir {

BasicBlock::BasicBlock(const llvm::BasicBlock* bb, SlotTracker* tracker, VariableFactory* factory)
        : instance_(bb),
          tracker_(tracker),
          factory_(factory),
          inputChanged_(false),
          atFixpoint_(false),
          visited_(false) {
    inputState_ = std::make_shared<State>(factory_);
    outputState_ = std::make_shared<State>(factory_);
}

const llvm::BasicBlock* BasicBlock::getInstance() const {
    return instance_;
}

SlotTracker& BasicBlock::getSlotTracker() const {
    return *tracker_;
}

BasicBlock::State::Ptr BasicBlock::getOutputState() const {
    return outputState_;
}

std::string BasicBlock::getName() const {
    return (instance_->hasName()) ? instance_->getName().str() : tracker_->getLocalName(instance_);
}

std::string BasicBlock::toString() const {
    std::ostringstream ss;
    ss << *this;
    return ss.str();
}

std::string BasicBlock::outputToString() const {
    std::ostringstream ss;
    ss << getName() << " output:" << std::endl;
    ss << outputState_->toString();
    ss << std::endl;
    return ss.str();
}

std::string BasicBlock::inputToString() const {
    std::ostringstream ss;
    ss << getName() << " input:" << std::endl;
    ss << inputState_->toString();
    ss << std::endl;
    return ss.str();
}


bool BasicBlock::empty() const {
    return inputState_->empty() && outputState_->empty();
}

bool BasicBlock::atFixpoint() {
    if (outputState_->empty() && not visited_) return false;
    else if (not inputChanged_) return atFixpoint_;
    else {
        atFixpoint_ = outputState_->equals(inputState_);
        inputChanged_ = false;
        return atFixpoint_;
    }
}

bool BasicBlock::isVisited() const {
    return visited_;
}

void BasicBlock::setVisited() {
    visited_ = true;
}

void BasicBlock::mergeToInput(State::Ptr input) {
    inputState_->joinWith(input);
    inputChanged_ = true;
}

void BasicBlock::addToInput(const llvm::Value* value, AbstractDomain::Ptr domain) {
    inputState_->assign(value, domain);
    inputChanged_ = true;
}

void BasicBlock::addToInput(const llvm::Instruction* inst, AbstractDomain::Ptr domain) {
    inputState_->assign(inst, domain);
    inputChanged_ = true;
}

AbstractDomain::Ptr BasicBlock::getDomainFor(const llvm::Value* value) {
    return outputState_->get(value);
}

void BasicBlock::mergeOutputWithInput() {
    outputState_->joinWith(inputState_);
}

std::vector<const llvm::Value*> BasicBlock::getGlobals() const {
    return util::viewContainer(globals_)
            .map([](auto&& a){ return a.first; })
            .toVector();
}

void BasicBlock::updateGlobals(const std::map<const llvm::Value*, AbstractDomain::Ptr>& globals) {
    for (auto&& it : globals_) {
        auto global = globals.at(it.first);
        ASSERT(global, "No value for global in map");
        if (not it.second->equals(global)) {
            it.second = it.second->join(global);
        }
    }
}

std::ostream& operator<<(std::ostream& s, const BasicBlock& b) {
    s << b.getName() << ":";
    for (auto&& it : util::viewContainer(*b.getInstance())) {
        auto value = llvm::cast<llvm::Value>(&it);
        auto domain = b.getOutputState()->get(value);
        if (not domain) continue;
        s << std::endl << "  "
          << b.getSlotTracker().getLocalName(value) << " = "
                                                    << domain->toString();
        s.flush();
    }
    s << std::endl;
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock& b) {
    s << b.getName() << ":";
    for (auto&& it : util::viewContainer(*b.getInstance())) {
        auto value = llvm::cast<llvm::Value>(&it);
        auto domain = b.getOutputState()->get(value);
        if (not domain) continue;

        s << endl << "  "
          << b.getSlotTracker().getLocalName(value) << " = "
          << domain->toString();
        s.flush();
    }
    s << endl;
    return s;
}

std::ostream& operator<<(std::ostream& s, const BasicBlock* b) {
    s << *b;
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock* b) {
    s << *b;
    return s;
}

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
