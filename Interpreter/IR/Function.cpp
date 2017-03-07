//
// Created by abdullin on 2/10/17.
//

#include "Function.h"
#include "Util/collections.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Function::Function(Environment::Ptr environment, const llvm::Function* function) : environment_(environment),
                                                                                   instance_(function),
                                                                                   tracker_(instance_) {
    inputState_ = State::Ptr{ new State() };
    outputState_ = State::Ptr{ new State() };
    for (auto&& block : util::viewContainer(*instance_)) {
        auto&& aiBlock = BasicBlock(&block, &tracker_);
        blocks_.insert({&block, aiBlock});
    }

    // adding global variables
    for (auto&& it : environment_->getModule()->getGlobalList()) {
        auto&& globalDomain = environment_->getDomainFactory().get(*it.getType());
        if (globalDomain) inputState_->addGlobalVariable(&it, globalDomain);
    }

    // adding return value
    auto&& returnType = instance_->getReturnType();
    if (not returnType->isVoidTy()) {
        auto&& retDomain = environment_->getDomainFactory().get(*returnType);
        if (retDomain) outputState_->setReturnValue(retDomain);
    }
}

const llvm::Function* Function::getInstance() const {
    return instance_;
}

const std::vector<Domain::Ptr>& Function::getArguments() const {
    return arguments_;
}

const Function::BlockMap& Function::getBasicBlocks() const {
    return blocks_;
}

State::Ptr Function::getInputState() const {
    return inputState_;
}

State::Ptr Function::getOutputState() const {
    return outputState_;
}

Domain::Ptr Function::getReturnValue() const {
    return outputState_->getReturnValue();
}

const BasicBlock* Function::getBasicBlock(const llvm::BasicBlock* bb) const {
    if (auto&& opt = util::at(blocks_, bb))
        return &opt.getUnsafe();
    return nullptr;
}

bool Function::empty() const {
    return blocks_.empty();
}

std::string Function::getName() const {
    return instance_->getName().str();
}

std::string Function::toString() const {
    std::ostringstream ss;
    ss << "--- Function \"";
    ss << getName() << "\" ---";

    if (not arguments_.empty()) {
        ss << std::endl << "Args: ";
        bool comma = false;
        for (auto&& it : arguments_) {
            if (comma) ss << ", ";
            ss << it;
            comma = true;
        }
        ss << std::endl;
    }

    for (auto&& it : util::viewContainer(*instance_)) {
        ss << std::endl << getBasicBlock(&it)->toString();
    }

    ss << std::endl << "Retval = ";
    if (outputState_->getReturnValue())
        ss << outputState_->getReturnValue()->toString();
    else ss << "void";

    return ss.str();
}

std::vector<const BasicBlock*> Function::getPredecessorsFor(const llvm::BasicBlock* bb) const {
    std::vector<const BasicBlock*> predecessors;
    for (auto it = pred_begin(bb), et = pred_end(bb); it != et; ++it) {
        predecessors.push_back(getBasicBlock(*it));
    }
    return std::move(predecessors);
}

std::vector<const BasicBlock*> Function::getSuccessorsFor(const llvm::BasicBlock* bb) const {
    std::vector<const BasicBlock*> successors;
    for (auto it = succ_begin(bb), et = succ_end(bb); it != et; ++it) {
        successors.push_back(getBasicBlock(*it));
    }
    return std::move(successors);
}

void Function::setArguments(const std::vector<Domain::Ptr>& args) {
    ASSERT(instance_->isVarArg() || instance_->getArgumentList().size() == args.size(), "Wrong number of arguments");
    arguments_.clear();

    // adding function arguments to input state
    auto&& it = instance_->getArgumentList().begin();
    for (auto i = 0U; i < args.size(); ++i) {
        if (args[i]) {
            inputState_->addLocalVariable(it, args[i]);
            arguments_.push_back(args[i]);
        }
        if (++it == instance_->getArgumentList().end()) break;
    }

    // merge the front block's input state with function input state
    getBasicBlock(&instance_->front())->getInputState()->merge(inputState_);
}

void Function::addCall(const llvm::Value* call, Function::Ptr function) {
    auto&& inst = llvm::dyn_cast<llvm::CallInst>(call);
    ASSERT(inst && inst->getParent()->getParent() == instance_, "Unknown call instruction");

    callMap_[call] = function;
}

const Function::CallMap& Function::getCallMap() const {
    return callMap_;
}

bool Function::atFixpoint() const {
    for (auto& block : blocks_) {
        if (not block.second.atFixpoint()) return false;
    }
    return true;
}

const SlotTracker& Function::getSlotTracker() const {
    return tracker_;
}

std::ostream& operator<<(std::ostream& s, const Function& f) {
    s << f.toString();
    return s;
}

std::ostream& operator<<(std::ostream& s, const Function* f) {
    s << *f;
    return s;
}

std::ostream& operator<<(std::ostream& s, const Function::Ptr f) {
    s << f.get();
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function& f) {
    s << f.toString();
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function* f) {
    s << *f;
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function::Ptr f) {
    s << f.get();
    return s;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"