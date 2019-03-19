//
// Created by abdullin on 2/9/17.
//

#ifndef BOREALIS_BASICBLOCK_H
#define BOREALIS_BASICBLOCK_H

#include <map>

#include <llvm/IR/BasicBlock.h>

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "State.h"
#include "Util/slottracker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

class VariableFactory;

namespace ir {

class BasicBlock {
private:

    const llvm::BasicBlock* instance_;
    mutable SlotTracker* tracker_;
    VariableFactory* factory_;
    std::map<const llvm::Value*, AbstractDomain::Ptr> globals_;
    State::Ptr inputState_;
    State::Ptr outputState_;
    bool inputChanged_;
    bool atFixpoint_;
    bool visited_;

    friend class Function;

public:

    BasicBlock(const llvm::BasicBlock* bb, SlotTracker* tracker, VariableFactory* factory);
    BasicBlock(const BasicBlock&) = default;
    BasicBlock(BasicBlock&&) = default;

    const llvm::BasicBlock* getInstance() const;
    SlotTracker& getSlotTracker() const;
    State::Ptr getOutputState() const;
    std::vector<const llvm::Value*> getGlobals() const;

    std::string getName() const;
    std::string toString() const;
    std::string inputToString() const;
    std::string outputToString() const;

    void updateGlobals(const std::map<const llvm::Value*, AbstractDomain::Ptr>& globals);
    void mergeOutputWithInput();
    void mergeToInput(State::Ptr input);
    void addToInput(const llvm::Value* value, AbstractDomain::Ptr domain);
    void addToInput(const llvm::Instruction* inst, AbstractDomain::Ptr domain);
    AbstractDomain::Ptr getDomainFor(const llvm::Value* value);

    bool empty() const;
    bool atFixpoint();

    void setVisited();
    bool isVisited() const;

};

std::ostream& operator<<(std::ostream& s, const BasicBlock& b);
std::ostream& operator<<(std::ostream& s, const BasicBlock* b);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock& b);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock* b);

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

#endif //BOREALIS_BASICBLOCK_H
