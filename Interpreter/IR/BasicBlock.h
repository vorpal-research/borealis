//
// Created by abdullin on 2/9/17.
//

#ifndef BOREALIS_BASICBLOCK_H
#define BOREALIS_BASICBLOCK_H

#include <map>

#include <llvm/IR/BasicBlock.h>

#include "IRState.h"
#include "Util/slottracker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

class BasicBlock {
private:

    const llvm::BasicBlock* instance_;
    mutable SlotTracker* tracker_;
    DomainFactory* factory_;
    std::map<const llvm::Value*, Domain::Ptr> globals_;
    IRState::Ptr inputState_;
    IRState::Ptr outputState_;
    bool inputChanged_;
    bool atFixpoint_;
    bool visited_;

    friend class Function;

public:

    BasicBlock(const llvm::BasicBlock* bb, SlotTracker* tracker, DomainFactory* factory);
    BasicBlock(const BasicBlock&) = default;
    BasicBlock(BasicBlock&&) = default;

    const llvm::BasicBlock* getInstance() const;
    SlotTracker& getSlotTracker() const;
    IRState::Ptr getOutputState() const;
    std::vector<const llvm::Value*> getGlobals() const;

    std::string getName() const;
    std::string toString() const;
    std::string inputToString() const;
    std::string outputToString() const;

    void updateGlobals(const std::map<const llvm::Value*, Domain::Ptr>& globals);
    void mergeOutputWithInput();
    void mergeToInput(IRState::Ptr input);
    void addToInput(const llvm::Value* value, Domain::Ptr domain);
    void addToInput(const llvm::Instruction* inst, Domain::Ptr domain);
    Domain::Ptr getDomainFor(const llvm::Value* value);

    bool empty() const;
    bool checkGlobalsChanged(const std::map<const llvm::Value*, Domain::Ptr>& globals) const;
    /// Needs a vector of current globals, to check if there were changed
    bool atFixpoint(const std::map<const llvm::Value*, Domain::Ptr>& globals);

    void setVisited();
    bool isVisited() const;

};

std::ostream& operator<<(std::ostream& s, const BasicBlock& b);
std::ostream& operator<<(std::ostream& s, const BasicBlock* b);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock& b);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock* b);

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

#endif //BOREALIS_BASICBLOCK_H
