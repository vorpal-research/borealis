//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_STATE_H
#define BOREALIS_STATE_H

#include <unordered_map>

#include <llvm/IR/Value.h>

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/DomainStorage.hpp"
#include "Util/cow_map.hpp"
#include "Util/slottracker.h"

namespace borealis {
namespace absint {
namespace ir {

class State : public std::enable_shared_from_this<State> {
public:

    using VariableMap = util::cow_map<const llvm::Value*, AbstractDomain::Ptr>;
    using BlockMap = std::unordered_map<const llvm::BasicBlock*, VariableMap>;
    using Ptr = std::shared_ptr<State>;

    explicit State(SlotTracker* tracker);
    State(const State& other);

    bool equals(const State* other) const;
    friend bool operator==(const State& lhv, const State& rhv);

    void addVariable(const llvm::Value* val, AbstractDomain::Ptr domain);
    void addVariable(const llvm::Instruction* inst, AbstractDomain::Ptr domain);
    void setReturnValue(AbstractDomain::Ptr domain);
    void mergeToReturnValue(AbstractDomain::Ptr domain);

    const State::BlockMap& getLocals() const;
    AbstractDomain::Ptr getReturnValue() const;

    void merge(State::Ptr other);
    AbstractDomain::Ptr find(const llvm::Value* val) const;

    bool empty() const;
    std::string toString() const;

private:

    void mergeVariables(State::Ptr other);
    void mergeReturnValue(State::Ptr other);

    VariableMap arguments_;
    BlockMap locals_;
    AbstractDomain::Ptr retval_;
    SlotTracker* tracker_;
};

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_STATE_H
