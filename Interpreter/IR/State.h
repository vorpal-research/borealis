//
// Created by abdullin on 2/7/17.
//

#ifndef BOREALIS_STATE_H
#define BOREALIS_STATE_H

#include <unordered_map>

#include <llvm/IR/Value.h>

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Util/cow_map.hpp"
#include "Util/slottracker.h"

namespace borealis {
namespace absint {

class VariableFactory;
class DomainStorage;

namespace ir {

class State : public std::enable_shared_from_this<State> {
public:

    using Ptr = std::shared_ptr<State>;

    State(SlotTracker* tracker, VariableFactory* vf);
    State(SlotTracker* tracker, std::shared_ptr<DomainStorage> storage);
    State(const State& other);

    bool equals(const State* other) const;
    friend bool operator==(const State& lhv, const State& rhv);

    AbstractDomain::Ptr get(const llvm::Value* x) const;
    void assign(const llvm::Value* x, const llvm::Value* y);
    void assign(const llvm::Value* v, AbstractDomain::Ptr domain);
    void apply(llvm::BinaryOperator::BinaryOps op, const llvm::Value* x, const llvm::Value* y, const llvm::Value* z);
    void apply(llvm::CmpInst::Predicate op, const llvm::Value* x, const llvm::Value* y, const llvm::Value* z);
    void apply(CastOperator op, const llvm::Value* x, const llvm::Value* y);
    void load(const llvm::Value* x, const llvm::Value* ptr);
    void store(const llvm::Value* ptr, const llvm::Value* x);
    void gep(const llvm::Value* x, const llvm::Value* ptr, const std::vector<const llvm::Value*>& shifts);

    void allocate(const llvm::Value* x, const llvm::Value* size);

    void merge(State::Ptr other);

    bool empty() const;
    std::string toString() const;

    std::pair<State::Ptr, State::Ptr> split(const llvm::Value* condition) const;

private:

    SlotTracker* tracker_;
    std::shared_ptr<DomainStorage> storage_;
};

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_STATE_H
