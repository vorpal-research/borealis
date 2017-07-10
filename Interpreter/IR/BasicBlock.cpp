//
// Created by abdullin on 2/9/17.
//

#include "BasicBlock.h"
#include "Interpreter/Domain/DomainFactory.h"

#include "Util/collections.hpp"
#include "Util/util.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

BasicBlock::BasicBlock(const llvm::BasicBlock* bb, SlotTracker* tracker, DomainFactory* factory)
        : instance_(bb),
          tracker_(tracker),
          factory_(factory),
          inputChanged_(false),
          atFixpoint_(false),
          visited_(false) {
    inputState_ = State::Ptr{ new State(tracker_) };
    outputState_ = State::Ptr{ new State(tracker_) };

    // find all global variables, that this function depends on
    for (auto&& inst : util::viewContainer(*instance_)
            .map([](const llvm::Instruction& i) -> llvm::Instruction& { return const_cast<llvm::Instruction&>(i); })) {
        for (auto&& op : util::viewContainer(inst.operand_values())
                .map([](llvm::Value* v) -> llvm::Value* {
                    if (auto&& gepOp = llvm::dyn_cast<llvm::GEPOperator>(v))
                        return gepOp->getPointerOperand();
                    else return v; })
                .filter(llvm::isaer<llvm::GlobalVariable>())) {
            globals_.insert({ op, factory_->getBottom(*op->getType()) });
        }
    }
}

const llvm::BasicBlock* BasicBlock::getInstance() const {
    return instance_;
}

SlotTracker& BasicBlock::getSlotTracker() const {
    return *tracker_;
}

State::Ptr BasicBlock::getOutputState() const {
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

std::string BasicBlock::toFullString() const {
    std::ostringstream ss;
    ss << getName() << " output:";
    ss << outputState_->toString();
    ss << std::endl;
    return ss.str();
}

std::string BasicBlock::inputToString() const {
    std::ostringstream ss;
    ss << getName() << " input:";
    ss << inputState_->toString();
    ss << std::endl;
    return ss.str();
}


bool BasicBlock::empty() const {
    return inputState_->empty() && outputState_->empty();
}

bool BasicBlock::atFixpoint(const std::map<const llvm::Value*, Domain::Ptr>& globals) {
    if (empty()) return false;
    if (checkGlobalsChanged(globals)) return false;
    if (not inputChanged_) return atFixpoint_;
    else {
        atFixpoint_ = inputState_->equals(outputState_.get());
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

void BasicBlock::addPredecessor(BasicBlock* pred) {
    predecessors_.push_back(pred);
}

void BasicBlock::addSuccessor(BasicBlock* succ) {
    successors_.push_back(succ);
}

void BasicBlock::mergeToInput(State::Ptr input) {
    inputState_->merge(input);
    inputChanged_ = true;
}

void BasicBlock::addToInput(const llvm::Value* value, Domain::Ptr domain) {
    inputState_->addVariable(value, domain);
    inputChanged_ = true;
}

Domain::Ptr BasicBlock::getDomainFor(const llvm::Value* value) {
    return outputState_->find(value);
}

void BasicBlock::mergeOutputWithInput() {
    outputState_->merge(inputState_);
}

std::vector<const llvm::Value*> BasicBlock::getGlobals() const {
    return util::viewContainer(globals_)
            .map([](auto&& a){ return a.first; })
            .toVector();
}

void BasicBlock::updateGlobals(const std::map<const llvm::Value*, Domain::Ptr>& globals) {
    for (auto&& it : globals_) {
        auto global = globals.at(it.first);
        ASSERT(global, "No value for global in map");
        if (not it.second->equals(global.get())) {
            it.second = it.second->join(global);
        }
    }
}

bool BasicBlock::checkGlobalsChanged(const std::map<const llvm::Value*, Domain::Ptr>& globals) const {
    for (auto&& it : globals_) {
        auto global = globals.at(it.first);
        ASSERT(global, "No value for global in map");
        if (not it.second->equals(global.get())) {
            return true;
        }
    }
    return false;
}

std::ostream& operator<<(std::ostream& s, const BasicBlock& b) {
    s << b.getName() << ":";
    for (auto&& it : util::viewContainer(*b.getInstance())) {
        auto value = llvm::cast<llvm::Value>(&it);
        auto domain = b.getOutputState()->find(value);
        if (not domain) continue;
        s << std::endl << "  "
          << b.getSlotTracker().getLocalName(value) << " = "
          << domain->toPrettyString("  ");
        s.flush();
    }
    s << std::endl;
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock& b) {
    s << b.getName() << ":";
    for (auto&& it : util::viewContainer(*b.getInstance())) {
        auto value = llvm::cast<llvm::Value>(&it);
        auto domain = b.getOutputState()->find(value);
        if (not domain) continue;

        s << endl << "  "
          << b.getSlotTracker().getLocalName(value) << " = "
          << domain->toPrettyString("  ");
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

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
