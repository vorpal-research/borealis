//
// Created by abdullin on 2/10/17.
//

#ifndef BOREALIS_FUNCTION_H
#define BOREALIS_FUNCTION_H

#include <map>
#include <unordered_map>

#include "BasicBlock.h"
#include "Interpreter/State.h"
#include "Util/slottracker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

class Function: public std::enable_shared_from_this<Function> {
public:

    using Ptr = std::shared_ptr<Function>;
    using BlockMap = std::unordered_map<const llvm::BasicBlock*, BasicBlock>;

private:

    const llvm::Function* instance_;
    mutable SlotTracker* tracker_;
    DomainFactory* factory_;
    std::vector<Domain::Ptr> arguments_;
    std::map<const llvm::Value*, Domain::Ptr> globals_;
    BlockMap blocks_;
    State::Ptr inputState_;
    State::Ptr outputState_;

protected:
    /// Assumes that llvm::Function is not a declaration
    Function(const llvm::Function* function, DomainFactory* factory, SlotTracker* st);

    friend class Module;

public:
    bool isVisited() const;
    bool hasAddressTaken() const;
    const llvm::FunctionType* getType() const;
    const llvm::Function* getInstance() const;
    const std::vector<Domain::Ptr>& getArguments() const;
    std::vector<const llvm::Value*> getGlobals() const;
    const BlockMap& getBasicBlocks() const;
    Domain::Ptr getReturnValue() const;

    Domain::Ptr getDomainFor(const llvm::Value* value, const llvm::BasicBlock* location);

    /// Assumes that @args[i] corresponds to i-th argument of the function
    /// Returns true, if arguments were updated and function should be reinterpreted
    bool updateArguments(const std::vector<Domain::Ptr>& args);
    /// Returns true, if globals were updated and function should be reinterpreted
    bool updateGlobals(const std::map<const llvm::Value*, Domain::Ptr>& globals);
    void mergeToOutput(State::Ptr state);

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

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

#endif //BOREALIS_FUNCTION_H
