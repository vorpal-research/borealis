//
// Created by abdullin on 2/10/17.
//

#include "Interpreter/Domain/DomainFactory.h"
#include "Function.h"
#include "Util/collections.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Function::Function(const llvm::Function* function, DomainFactory* factory, SlotTracker* st)
        : instance_(function),
          tracker_(st),
          factory_(factory) {
    inputState_ = State::Ptr{ new State() };
    outputState_ = State::Ptr{ new State() };
    util::viewContainer(instance_->getArgumentList()).foreach([&](const llvm::Argument& arg) {
        arguments_.push_back(nullptr);
    });
    for (auto&& block : util::viewContainer(*instance_)) {
        auto&& aiBlock = BasicBlock(&block, tracker_);
        blocks_.insert( {&block, aiBlock} );
        blockVector_.push_back(&blocks_.at(&block));
    }

    for (auto&& it : blocks_) {
        for (auto bb = pred_begin(it.first), et = pred_end(it.first); bb != et; ++bb) {
            it.second.addPredecessor(getBasicBlock(*bb));
        }
        for (auto bb = succ_begin(it.first), et = succ_end(it.first); bb != et; ++bb) {
            it.second.addSuccessor(getBasicBlock(*bb));
        }
    }

    // adding return value
    auto&& returnType = instance_->getReturnType();
    if (not returnType->isVoidTy()) {
        auto&& retDomain = factory_->getBottom(*returnType);
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

BasicBlock* Function::getEntryNode() const {
    return getBasicBlock(&instance_->getEntryBlock());
}

BasicBlock* Function::getBasicBlock(const llvm::BasicBlock* bb) const {
    if (auto&& opt = util::at(blocks_, bb))
        return const_cast<BasicBlock*>(&opt.getUnsafe());
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
        auto i = 0U;
        for (auto&& it : instance_->args()) {
            ss << std::endl << tracker_->getLocalName(&it) << " = " << arguments_[i++];
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

const SlotTracker& Function::getSlotTracker() const {
    return *tracker_;
}

bool Function::updateArguments(const std::vector<Domain::Ptr>& args) {
    ASSERT(instance_->isVarArg() || arguments_.size() == args.size(), "Wrong number of arguments");
    // This is generally fucked up
    if (arguments_.size() == 0)
        return not getEntryNode()->isVisited();

    bool changed = false;
    // adding function arguments to input state
    auto&& it = instance_->getArgumentList().begin();
    for (auto i = 0U; i < arguments_.size(); ++i, ++it) {
        ASSERT(args[i], "Nullptr in functions arguments");

        auto arg = args[i];
        if (arguments_[i]) {
            arg = arguments_[i]->widen(arg);
            changed |= not arguments_[i]->equals(arg.get());
        } else {
            changed = true;
        }
        arguments_[i] = arg;
        inputState_->addVariable(it, arguments_[i]);
    }
    getEntryNode()->getInputState()->merge(inputState_);
    return changed;
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