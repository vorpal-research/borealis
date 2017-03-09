//
// Created by abdullin on 2/10/17.
//

#include "Function.h"
#include "Util/collections.hpp"

namespace borealis {
namespace absint {


Function::Function(const Environment* environment, const llvm::Function* function) : environment_(environment),
                                                                                   instance_(function) {
    inputState_ = std::make_shared<State>(State(environment_));
    outputState_ = std::make_shared<State>(State(environment_));
    for (auto&& block : util::viewContainer(*instance_)) {
        auto&& aiBlock = BasicBlock(environment_, &block);
        blocks_.insert({&block, aiBlock});
    }
    // TODO: add arguments to states
    // TODO: init basic blocks
    // TODO: add return value

    auto&& returnType = instance_->getReturnType();
    if (not returnType->isVoidTy()) {
        auto&& retDomain = environment_->getDomainFactory().get(*returnType);
        if (retDomain) outputState_->setReturnValue(retDomain);
    }
}

const llvm::Function* Function::getInstance() const {
    return instance_;
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
    ss << "Function ";
    ss << instance_->getName().str() << std::endl;
    for (auto&& it : util::viewContainer(blocks_))
        ss << it.second.toString() << std::endl;

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

}   /* namespace absint */
}   /* namespace borealis */