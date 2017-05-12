//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_STATE_H
#define BOREALIS_STATE_H

#include <unordered_map>

#include <llvm/IR/Value.h>

#include "Interpreter/Domain/Domain.h"
#include "Util/slottracker.h"

namespace borealis {
namespace absint {

class State : public std::enable_shared_from_this<State> {
public:

    using Map = std::map<const llvm::Value*, Domain::Ptr>;
    using Ptr = std::shared_ptr<State>;

    State();
    State(const State& other);

    bool equals(const State* other) const;
    friend bool operator==(const State& lhv, const State& rhv);

    void addVariable(const llvm::Value* val, Domain::Ptr domain);
    void setReturnValue(Domain::Ptr domain);
    void mergeToReturnValue(Domain::Ptr domain);

    const State::Map& getLocals() const;
    Domain::Ptr getReturnValue() const;

    void merge(State::Ptr other);
    void mergeVariables(State::Ptr other);
    void mergeReturnValue(State::Ptr other);
    Domain::Ptr find(const llvm::Value* val) const;

    bool empty() const;
    std::string toString(SlotTracker& tracker) const;

private:

    Map locals_;
    Domain::Ptr retval_;
};

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_STATE_H
