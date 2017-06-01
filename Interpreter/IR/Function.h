//
// Created by abdullin on 2/10/17.
//

#ifndef BOREALIS_FUNCTION_H
#define BOREALIS_FUNCTION_H

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
    using BlockVector = std::vector<BasicBlock*>;
    using CallMap = std::unordered_map<const llvm::Value*, Function::Ptr>;
    using iterator = BlockMap::iterator;
    using const_iterator = BlockMap::const_iterator;

private:

    const llvm::Function* instance_;
    mutable SlotTracker tracker_;
    DomainFactory* factory_;
    std::vector<Domain::Ptr> arguments_;
    BlockMap blocks_;
    BlockVector blockVector_;
    State::Ptr inputState_;
    State::Ptr outputState_;

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

    BasicBlock* getEntryNode() const;
    BasicBlock* getBasicBlock(const llvm::BasicBlock* bb) const;
    const SlotTracker& getSlotTracker() const;

    bool empty() const;
    bool atFixpoint();
    std::string getName() const;
    std::string toString() const;

    BlockVector& getVector() {
        return blockVector_;
    }

    auto begin() QUICK_RETURN(blockVector_.begin());
    auto begin() QUICK_CONST_RETURN(blockVector_.begin());
    auto end() QUICK_RETURN(blockVector_.end());
    auto end() QUICK_CONST_RETURN(blockVector_.end());

};

std::ostream& operator<<(std::ostream& s, const Function& f);
std::ostream& operator<<(std::ostream& s, const Function* f);
std::ostream& operator<<(std::ostream& s, const Function::Ptr f);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function& f);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function* f);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const Function::Ptr f);

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

#endif //BOREALIS_FUNCTION_H
