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
    inputState_ = std::make_shared<borealis::absint::State>(tracker_);
    outputState_ = std::make_shared<borealis::absint::State>(tracker_);
    for (auto i = 0U; i < instance_->getArgumentList().size(); ++i) arguments_.emplace_back(nullptr);

    // find all global variables, that this function depends on
    for (auto&& inst : util::viewContainer(*instance_)
            .flatten()
            .map([](const llvm::Instruction& i) -> llvm::Instruction& { return const_cast<llvm::Instruction&>(i); })) {
        for (auto&& op : util::viewContainer(inst.operand_values())
                .map([](llvm::Value* v) -> llvm::Value* {
                    if (auto&& gepOp = llvm::dyn_cast<llvm::GEPOperator>(v))
                        return gepOp->getPointerOperand();
                    else return v; })
                .filter([](llvm::Value* v) -> bool {
                    if (auto&& global = llvm::dyn_cast<llvm::GlobalVariable>(v)) {
                        return not global->isConstant();
                    } else return false;
                })) {
            globals_.insert({ op, factory_->getBottom(*op->getType()) });
        }
    }

    for (auto&& block : util::viewContainer(*instance_)) {
        auto&& aiBlock = BasicBlock(&block, tracker_, factory_);
        blocks_.insert( {&block, aiBlock} );
        blockVector_.emplace_back(&blocks_.at(&block));
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
    ss << *this;
    return ss.str();
}

SlotTracker& Function::getSlotTracker() const {
    return *tracker_;
}

bool Function::updateArguments(const std::vector<Domain::Ptr>& args) {
    ASSERT(instance_->isVarArg() || arguments_.size() == args.size(), "Wrong number of arguments");
    // This is generally fucked up
    if (arguments_.empty())
        return not getEntryNode()->isVisited();

    bool changed = false;
    // adding function arguments to input state
    auto&& it = instance_->getArgumentList().begin();
    for (auto i = 0U; i < arguments_.size(); ++i, ++it) {
        ASSERT(args[i], "Nullptr in functions arguments");

        auto arg = args[i];
        if (arguments_[i]) {
            arg = (arguments_[i]->clone())->widen(arg);
            changed |= not arguments_[i]->equals(arg.get());
        } else {
            changed = true;
        }
        arguments_[i] = arg;
        inputState_->addVariable(it, arguments_[i]->clone());
    }
    getEntryNode()->mergeToInput(inputState_);
    return changed;
}

Domain::Ptr Function::getDomainFor(const llvm::Value* value, const llvm::BasicBlock* location) {
    return getBasicBlock(location)->getDomainFor(value);
}

void Function::mergeToOutput(State::Ptr state) {
    outputState_->merge(state);
}

std::vector<const llvm::Value*> Function::getGlobals() const {
    return util::viewContainer(globals_)
            .map([](auto&& a){ return a.first; })
            .toVector();
}

bool Function::updateGlobals(const std::map<const llvm::Value*, Domain::Ptr>& globals) {
    bool updated = false;
    for (auto&& it : globals_) {
        auto global = globals.at(it.first);
        ASSERT(global, "No value for global in map");
        if (not it.second->equals(global.get())) {
            it.second = it.second->join(global);
            updated = true;
        }
    }
    return updated;
}

size_t Function::hashCode() const {
    return (size_t) getInstance();
}

bool Function::equals(Function* other) const {
    return getInstance() == other->getInstance();
}

const llvm::FunctionType* Function::getType() const {
    return llvm::cast<llvm::FunctionType>(instance_->getType()->getPointerElementType());
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

std::ostream& operator<<(std::ostream& s, const Function::Ptr f) {
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
                s << endl << f.getSlotTracker().getLocalName(&it) << " = " << arguments[i];
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

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function::Ptr f) {
    s << f.get();
    return s;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"