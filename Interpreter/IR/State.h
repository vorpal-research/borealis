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

    using Ptr = std::shared_ptr<State>;

    explicit State(SlotTracker* tracker, VariableFactory* vf);
    State(const State& other);

    bool equals(const State* other) const;
    friend bool operator==(const State& lhv, const State& rhv);

    AbstractDomain::Ptr get(const llvm::Value* x) const;
    void assign(const llvm::Value* x, const llvm::Value* y);
    void assign(const llvm::Value* v, AbstractDomain::Ptr domain);
    void apply(llvm::BinaryOperator::BinaryOps op, const llvm::Value* x, const llvm::Value* y, const llvm::Value* z);
    void apply(llvm::CmpInst::Predicate op, const llvm::Value* x, const llvm::Value* y, const llvm::Value* z);
    void load(const llvm::Value* x, const llvm::Value* ptr);
    void store(const llvm::Value* ptr, const llvm::Value* x);
    void gep(const llvm::Value* x, const llvm::Value* ptr, const std::vector<const llvm::Value*>& shifts);

    void merge(State::Ptr other);

    bool empty() const;
    std::string toString() const;

private:

    SlotTracker* tracker_;
    DomainStorage::Ptr storage_;
};

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_STATE_H
