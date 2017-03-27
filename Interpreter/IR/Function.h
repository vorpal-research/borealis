//
// Created by abdullin on 2/10/17.
//

#ifndef BOREALIS_FUNCTION_H
#define BOREALIS_FUNCTION_H

#include "BasicBlock.h"
#include "Interpreter/State.h"
#include "Util/slottracker.h"

namespace borealis {
namespace absint {

class Function: public std::enable_shared_from_this<Function> {
public:
    using Ptr = std::shared_ptr<Function>;
    using BlockMap = std::unordered_map<const llvm::BasicBlock*, BasicBlock>;
    using CallMap = std::unordered_map<const llvm::Value*, Function::Ptr>;

protected:
    /// Assumes that llvm::Function is not a declaration
    Function(const llvm::Function* function, DomainFactory* factory);

    friend class Module;

public:
    const llvm::Function* getInstance() const;
    const std::vector<Domain::Ptr>& getArguments() const;
    const BlockMap& getBasicBlocks() const;

    State::Ptr getInputState() const;
    State::Ptr getOutputState() const;
    Domain::Ptr getReturnValue() const;

    /// Assumes that @args[i] corresponds to i-th argument of the function
    void setArguments(const std::vector<Domain::Ptr>& args);

    BasicBlock* getBasicBlock(const llvm::BasicBlock* bb) const;
    const SlotTracker& getSlotTracker() const;

    bool empty() const;
    bool atFixpoint();
    std::string getName() const;
    std::string toString() const;

    std::vector<const BasicBlock*> getPredecessorsFor(const llvm::BasicBlock* bb) const;
    std::vector<const BasicBlock*> getPredecessorsFor(const BasicBlock* bb) const {
        return std::move(getPredecessorsFor(bb->getInstance()));
    }
    std::vector<const BasicBlock*> getSuccessorsFor(const llvm::BasicBlock* bb) const;
    std::vector<const BasicBlock*> getSuccessorsFor(const BasicBlock* bb) const {
        return std::move(getSuccessorsFor(bb->getInstance()));
    }

private:

    const llvm::Function* instance_;
    mutable SlotTracker tracker_;
    DomainFactory* factory_;
    std::vector<Domain::Ptr> arguments_;
    BlockMap blocks_;
    State::Ptr inputState_;
    State::Ptr outputState_;
};

std::ostream& operator<<(std::ostream& s, const Function& f);
std::ostream& operator<<(std::ostream& s, const Function* f);
std::ostream& operator<<(std::ostream& s, const Function::Ptr f);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function& f);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function* f);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function::Ptr f);

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_FUNCTION_H
