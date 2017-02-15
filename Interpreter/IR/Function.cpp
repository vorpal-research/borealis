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
                                                                                   instance_(function) {
    inputState_ = std::make_shared<State>(State(environment_));
    outputState_ = std::make_shared<State>(State(environment_));
    for (auto&& block : util::viewContainer(*instance_)) {
        auto&& aiBlock = BasicBlock(environment_, &block);
        blocks_.insert({&block, aiBlock});
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
    ss << "--- Function \"";
    ss << instance_->getName().str() << "\" ---";
    for (auto&& it : util::viewContainer(blocks_))
        ss << std::endl << it.second.toString();

    ss << std::endl << "Retval = ";
    ss << outputState_->getReturnValue()->toString();

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
    if (args.empty()) {
        for (auto&& arg : util::viewContainer(instance_->getArgumentList())) {
            auto&& argDomain = environment_->getDomainFactory().get(&arg);
            if (argDomain) inputState_->addLocalVariable(&arg, argDomain);
        }
    } else {
        ASSERT(instance_->getArgumentList().size() == args.size(), "Wrong number of arguments");

        // adding function arguments to input state
        auto&& it = instance_->getArgumentList().begin();
        for (auto i = 0U; i < args.size(); ++i) {
            inputState_->addLocalVariable(it, args[i]);
            ++it;
        }
    }
    // merging the fron block's input state with function input state
    getBasicBlock(&instance_->front())->getInputState()->merge(inputState_);
}

std::ostream& operator<<(std::ostream& s, const Function& f) {
    s << f.toString();
    return s;
}

std::ostream& operator<<(std::ostream& s, const Function* f) {
    s << *f;
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

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"