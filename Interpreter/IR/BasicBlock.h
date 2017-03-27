//
// Created by abdullin on 2/9/17.
//

#ifndef BOREALIS_BASICBLOCK_H
#define BOREALIS_BASICBLOCK_H

#include <llvm/IR/BasicBlock.h>

#include "Interpreter/State.h"
#include "Util/slottracker.h"

namespace borealis {
namespace absint {

class BasicBlock {
public:

    BasicBlock(const llvm::BasicBlock* bb, SlotTracker* tracker);

    const llvm::BasicBlock* getInstance() const;
    State::Ptr getInputState() const;
    State::Ptr getOutputState() const;

    std::string getName() const;
    std::string toString() const;

    bool empty() const;
    bool atFixpoint();

    void setVisited();
    bool isVisited() const;

private:

    const llvm::BasicBlock* instance_;
    mutable SlotTracker* tracker_;
    State::Ptr inputState_;
    State::Ptr outputState_;
    bool atFixpoint_;
    bool visited_;
};

std::ostream& operator<<(std::ostream& s, const BasicBlock& b);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock& b);

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_BASICBLOCK_H
