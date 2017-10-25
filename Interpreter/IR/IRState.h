//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_STATE_H
#define BOREALIS_STATE_H

#include <unordered_map>

#include <llvm/IR/Value.h>

#include "Interpreter/Domain/Domain.h"
#include "Util/cow_map.hpp"
#include "Util/slottracker.h"

namespace borealis {
namespace absint {

class IRState : public std::enable_shared_from_this<IRState> {
public:

    using VariableMap = util::cow_map<const llvm::Value*, Domain::Ptr>;
    using BlockMap = std::unordered_map<const llvm::BasicBlock*, VariableMap>;
    using Ptr = std::shared_ptr<IRState>;

    explicit IRState(SlotTracker* tracker);
    IRState(const IRState& other);

    bool equals(const IRState* other) const;
    friend bool operator==(const IRState& lhv, const IRState& rhv);

    void addVariable(const llvm::Value* val, Domain::Ptr domain);
    void addVariable(const llvm::Instruction* inst, Domain::Ptr domain);
    void setReturnValue(Domain::Ptr domain);
    void mergeToReturnValue(Domain::Ptr domain);

    const IRState::BlockMap& getLocals() const;
    Domain::Ptr getReturnValue() const;

    void merge(IRState::Ptr other);
    Domain::Ptr find(const llvm::Value* val) const;

    bool empty() const;
    std::string toString() const;

private:

    void mergeVariables(IRState::Ptr other);
    void mergeReturnValue(IRState::Ptr other);

    VariableMap arguments_;
    BlockMap locals_;
    Domain::Ptr retval_;
    SlotTracker* tracker_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_STATE_H
