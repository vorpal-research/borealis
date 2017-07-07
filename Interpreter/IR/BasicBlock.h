//
// Created by abdullin on 2/9/17.
//

#ifndef BOREALIS_BASICBLOCK_H
#define BOREALIS_BASICBLOCK_H

#include <llvm/IR/BasicBlock.h>

#include "Interpreter/State.h"
#include "Util/slottracker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

class BasicBlock {
public:

    using BlockMap = std::vector<BasicBlock*>;
    // for GraphTraits
    using graph_iterator = BlockMap::iterator;
    using graph_const_iterator = BlockMap::const_iterator;

private:

    const llvm::BasicBlock* instance_;
    mutable SlotTracker* tracker_;
    State::Ptr inputState_;
    State::Ptr outputState_;
    BlockMap predecessors_;
    BlockMap successors_;
    bool visited_;

    void addPredecessor(BasicBlock* pred);
    void addSuccessor(BasicBlock* succ);

    friend class Function;

public:

    BasicBlock(const llvm::BasicBlock* bb, SlotTracker* tracker);

    const llvm::BasicBlock* getInstance() const;
    SlotTracker& getSlotTracker() const;
    State::Ptr getInputState() const;
    State::Ptr getOutputState() const;

    std::string getName() const;
    std::string toString() const;
    std::string inputToString() const;
    std::string toFullString() const;

    Domain::Ptr getDomainFor(const llvm::Value* value);

    bool empty() const;
    bool atFixpoint();

    void setVisited();
    bool isVisited() const;

    auto succ_begin() QUICK_RETURN(successors_.begin());
    auto succ_begin() QUICK_CONST_RETURN(successors_.begin());
    auto succ_end() QUICK_RETURN(successors_.end());
    auto succ_end() QUICK_CONST_RETURN(successors_.end());

    auto pred_begin() QUICK_RETURN(predecessors_.begin());
    auto pred_begin() QUICK_CONST_RETURN(predecessors_.begin());
    auto pred_end() QUICK_RETURN(predecessors_.end());
    auto pred_end() QUICK_CONST_RETURN(predecessors_.end());

};

std::ostream& operator<<(std::ostream& s, const BasicBlock& b);
std::ostream& operator<<(std::ostream& s, const BasicBlock* b);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock& b);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock* b);

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

#endif //BOREALIS_BASICBLOCK_H
