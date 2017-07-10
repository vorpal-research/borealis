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
    DomainFactory* factory_;
    std::map<const llvm::Value*, Domain::Ptr> globals_;
    State::Ptr inputState_;
    State::Ptr outputState_;
    BlockMap predecessors_;
    BlockMap successors_;
    bool inputChanged_;
    bool atFixpoint_;
    bool visited_;

    void addPredecessor(BasicBlock* pred);
    void addSuccessor(BasicBlock* succ);

    friend class Function;

public:

    BasicBlock(const llvm::BasicBlock* bb, SlotTracker* tracker, DomainFactory* factory);

    const llvm::BasicBlock* getInstance() const;
    SlotTracker& getSlotTracker() const;
    State::Ptr getOutputState() const;
    std::vector<const llvm::Value*> getGlobals() const;

    std::string getName() const;
    std::string toString() const;
    std::string inputToString() const;
    std::string toFullString() const;

    void updateGlobals(const std::map<const llvm::Value*, Domain::Ptr>& globals);
    void mergeOutputWithInput();
    void mergeToInput(State::Ptr input);
    void addToInput(const llvm::Value* value, Domain::Ptr domain);
    Domain::Ptr getDomainFor(const llvm::Value* value);

    bool empty() const;
    bool checkGlobalsChanged(const std::map<const llvm::Value*, Domain::Ptr>& globals) const;
    /// Needs a vector of current globals, to check if there were changed
    bool atFixpoint(const std::map<const llvm::Value*, Domain::Ptr>& globals);

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
