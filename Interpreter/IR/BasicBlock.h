//
// Created by abdullin on 2/9/17.
//

#ifndef BOREALIS_BASICBLOCK_H
#define BOREALIS_BASICBLOCK_H

#include <llvm/IR/BasicBlock.h>

#include "Interpreter/Environment.h"
#include "Interpreter/State.h"

namespace borealis {
namespace absint {

class BasicBlock {
public:

    BasicBlock(const Environment* environment, const llvm::BasicBlock* bb);

    const llvm::BasicBlock* getInstance() const;
    State::Ptr getInputState() const;
    State::Ptr getOutputState() const;

    std::string getName() const;
    std::string toString() const;

    bool atFixpoint() const;

private:

    const Environment* environment_;
    const llvm::BasicBlock* instance_;
    State::Ptr inputState_;
    State::Ptr outputState_;
    mutable bool atFixpoint_;
};

std::ostream& operator<<(std::ostream& s, const BasicBlock& b);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const BasicBlock& b);

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_BASICBLOCK_H
