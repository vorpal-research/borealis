//
// Created by abdullin on 2/10/17.
//

#ifndef BOREALIS_FUNCTION_H
#define BOREALIS_FUNCTION_H

#include "BasicBlock.h"
#include "Interpreter/Environment.h"
#include "Interpreter/State.h"

namespace borealis {
namespace absint {

class Function {
public:
    using BlockMap = std::unordered_map<const llvm::BasicBlock*, BasicBlock>;

    Function(const Environment* environment, const llvm::Function* function);

    const llvm::Function* getInstance() const;
    const BlockMap& getBasicBlocks() const;

    State::Ptr getInputState() const;
    State::Ptr getOutputState() const;


    const BasicBlock* getBasicBlock(const llvm::BasicBlock* bb) const;

    bool empty() const;
    std::string getName() const;
    std::string toString() const;

    std::vector<const BasicBlock*> getPredecessorsFor(const llvm::BasicBlock* bb) const;
    std::vector<const BasicBlock*> getPredecessorsFor(const BasicBlock& bb) const {
        return std::move(getPredecessorsFor(bb.getInstance()));
    }
    std::vector<const BasicBlock*> getSuccessorsFor(const llvm::BasicBlock* bb) const;
    std::vector<const BasicBlock*> getSuccessorsFor(const BasicBlock& bb) const {
        return std::move(getSuccessorsFor(bb.getInstance()));
    }

private:

    const Environment* environment_;
    const llvm::Function* instance_;
    BlockMap blocks_;
    State::Ptr inputState_;
    State::Ptr outputState_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_FUNCTION_H
