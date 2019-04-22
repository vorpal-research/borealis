//
// Created by abdullin on 2/10/17.
//

#include "DomainStorage.hpp"
#include "Interpreter/Domain/VariableFactory.hpp"
#include "Function.h"
#include "Util/collections.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ir {

Function::Function(const llvm::Function* function, VariableFactory* factory, SlotTracker* st)
        : instance_(function),
          tracker_(st),
          factory_(factory) {
    inputState_ = std::make_shared<State>(factory_);
    outputState_ = std::make_shared<State>(factory_);
    for (auto i = 0U; i < instance_->getArgumentList().size(); ++i) arguments_.emplace_back(nullptr);

    // find all global variables, that this function depends on
    for (auto&& inst : util::viewContainer(*instance_)
            .flatten()
            .map(ops::take_pointer)
            .map(llvm::dyn_caster<llvm::ReturnInst>())
            .filter()) {
        returnValue_ = inst->getReturnValue();
    }

    for (auto&& block : util::viewContainer(*instance_)) {
        blocks_.insert( {&block, std::move(BasicBlock{&block, tracker_, factory_}) });
    }

}

const llvm::Function* Function::getInstance() const {
    return instance_;
}

const std::vector<AbstractDomain::Ptr>& Function::getArguments() const {
    return arguments_;
}

const Function::BlockMap& Function::getBasicBlocks() const {
    return blocks_;
}

AbstractDomain::Ptr Function::getReturnValue() const {
    return (returnValue_ != nullptr) ? outputState_->get(returnValue_) : nullptr;
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
    ss << *this;
    return ss.str();
}

SlotTracker& Function::getSlotTracker() const {
    return *tracker_;
}

bool Function::updateArguments(const std::vector<AbstractDomain::Ptr>& args) {
    ASSERT(instance_->isVarArg() || arguments_.size() == args.size(), "Wrong number of arguments");

    bool changed = false;
    // adding function arguments to input state
    auto i = 0U;
    for (auto&& it : instance_->args()) {
        ASSERT(args[i], "Nullptr in functions arguments");

        if (not arguments_[i]) {
            arguments_[i] = args[i]->clone();
            changed = true;
            inputState_->assign(&it, args[i]);
        } else {
            // This is generally fucked up
            AbstractDomain::Ptr arg = args[i]->isTop() ?
                              args[i]->clone() :
                              (arguments_[i]->clone())->widen(args[i]);
            changed |= not arguments_[i]->equals(arg);
            inputState_->assign(&it, args[i]->widen(arguments_[i]));
            arguments_[i] = arg;
        }
        ++i;
    }
//    util::viewContainer(args)
//            .reverse()
//            .take(args.size() - i)
//            .foreach([](auto&& a) -> void { if (a->isMutable()) a->moveToTop(); });
    getEntryNode()->mergeToInput(inputState_);
    return changed;
}

AbstractDomain::Ptr Function::getDomainFor(const llvm::Value* value, const llvm::BasicBlock* location) {
    return getBasicBlock(location)->getDomainFor(value);
}

void Function::merge(State::Ptr state) {
    outputState_->joinWith(state);
}

size_t Function::hashCode() const {
    return (size_t) getInstance();
}

bool Function::equals(Function* other) const {
    return getInstance() == other->getInstance();
}

Type::Ptr Function::getType() const {
    return factory_->cast(instance_->getType()->getPointerElementType());
}

bool Function::isVisited() const {
    return getEntryNode()->isVisited();
}

bool Function::hasAddressTaken() const {
    return llvm::hasAddressTaken(*instance_);
}

std::ostream& operator<<(std::ostream& ss, const Function& f) {
    ss << "--- Function \"" << f.getName() << "\" ---";

    auto& arguments = f.getArguments();
    if (not arguments.empty()) {
        auto i = 0U;
        for (auto&& it : f.getInstance()->args()) {
            if (arguments[i]) {
                ss << std::endl << f.getSlotTracker().getLocalName(&it) << " = " << arguments[i];
            }
            ++i;
        }
        ss << std::endl;
    }

    for (auto&& it : util::viewContainer(*f.getInstance())) {
        if (not f.getBasicBlock(&it)->isVisited()) continue;
        ss << std::endl << f.getBasicBlock(&it);
        ss.flush();
    }

    ss << std::endl << "Retval = ";
    if (f.getReturnValue())
        ss << f.getReturnValue()->toString();
    else ss << "void";

    return ss;
}

std::ostream& operator<<(std::ostream& s, const Function* f) {
    s << *f;
    return s;
}

std::ostream& operator<<(std::ostream& s, Function::Ptr f) {
    s << f.get();
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function& f) {
    s << "--- Function \"" << f.getName() << "\" ---";

    auto& arguments = f.getArguments();
    if (not arguments.empty()) {
        auto i = 0U;
        for (auto&& it : f.getInstance()->args()) {
            if (arguments[i]) {
                s << endl << f.getSlotTracker().getLocalName(&it) << " = " << arguments[i]->toString();
            }
            ++i;
            s.flush();
        }
        s << endl;
    }

    for (auto&& it : util::viewContainer(*f.getInstance())) {
        if (not f.getBasicBlock(&it)->isVisited()) continue;
        s << endl << f.getBasicBlock(&it);
        s.flush();
    }

    s << endl << "Retval = ";
    if (f.getReturnValue())
        s << f.getReturnValue();
    else s << "void";

    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function* f) {
    s << *f;
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, Function::Ptr f) {
    s << f.get();
    return s;
}

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"