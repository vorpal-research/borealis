//
// Created by abdullin on 2/10/17.
//

#ifndef BOREALIS_FUNCTION_H
#define BOREALIS_FUNCTION_H

#include <map>
#include <unordered_map>

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/IR/BasicBlock.h"
#include "Util/slottracker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

class VariableFactory;

namespace ir {

class Function: public std::enable_shared_from_this<Function> {
public:

    using Ptr = std::shared_ptr<Function>;
    using BlockMap = std::unordered_map<const llvm::BasicBlock*, BasicBlock>;
    using State = BasicBlock::State;
    using StatePtr = BasicBlock::StatePtr;

private:

    const llvm::Function* instance_;
    const llvm::Value* returnValue_;
    mutable SlotTracker* tracker_;
    VariableFactory* factory_;
    std::vector<AbstractDomain::Ptr> arguments_;
    BlockMap blocks_;
    StatePtr inputState_;
    StatePtr outputState_;

public:
    /// Assumes that llvm::Function is not a declaration
    Function(const llvm::Function* function, VariableFactory* factory, SlotTracker* st);

    bool isVisited() const;
    bool hasAddressTaken() const;
    Type::Ptr getType() const;
    const llvm::Function* getInstance() const;
    const std::vector<AbstractDomain::Ptr>& getArguments() const;
    const BlockMap& getBasicBlocks() const;
    AbstractDomain::Ptr getReturnValue() const;

    AbstractDomain::Ptr getDomainFor(const llvm::Value* value, const llvm::BasicBlock* location);

    /// Assumes that @args[i] corresponds to i-th argument of the function
    /// Returns true, if arguments were updated and function should be reinterpreted
    bool updateArguments(const std::vector<AbstractDomain::Ptr>& args);
    void merge(StatePtr state);

    BasicBlock* getEntryNode() const;
    BasicBlock* getBasicBlock(const llvm::BasicBlock* bb) const;
    SlotTracker& getSlotTracker() const;

    bool equals(Function* other) const;
    size_t hashCode() const;
    bool empty() const;
    std::string getName() const;
    std::string toString() const;

};

struct FunctionHash {
    size_t operator()(Function::Ptr f) const noexcept {
        return f->hashCode();
    }
};

struct FunctionEquals {
    bool operator()(Function::Ptr lhv, Function::Ptr rhv) const noexcept {
        return lhv->equals(rhv.get());
    }
};

std::ostream& operator<<(std::ostream& s, const Function& f);
std::ostream& operator<<(std::ostream& s, const Function* f);
std::ostream& operator<<(std::ostream& s, Function::Ptr f);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function& f);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function* f);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, Function::Ptr f);

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

#endif //BOREALIS_FUNCTION_H
