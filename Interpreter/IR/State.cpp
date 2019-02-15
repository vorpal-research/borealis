//
// Created by abdullin on 2/7/17.
//

#include "State.h"
#include "Interpreter/Domain/DomainStorage.hpp"
#include "Util/collections.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ir {

State::State(borealis::SlotTracker* tracker, VariableFactory* vf) :
        tracker_(tracker), storage_(std::make_shared<DomainStorage>(vf)) {}

State::State(const State& other) :
        tracker_(other.tracker_), storage_(other.storage_->clone()) {}

bool State::equals(const State* other) const {
    return this->storage_->equals(other->storage_);
}

AbstractDomain::Ptr State::get(const llvm::Value* x) const {
    return storage_->get(x);
}

void State::assign(const llvm::Value* x, const llvm::Value* y) {
    storage_->assign(x, y);
}

void State::assign(const llvm::Value* v, AbstractDomain::Ptr domain) {
    storage_->assign(v, domain);
}

void State::apply(llvm::BinaryOperator::BinaryOps op, const llvm::Value* x, const llvm::Value* y, const llvm::Value* z) {
    storage_->apply(op, x, y, z);
}

void State::apply(llvm::CmpInst::Predicate op, const llvm::Value* x, const llvm::Value* y, const llvm::Value* z) {
    storage_->apply(op, x, y, z);
}

void State::apply(CastOperator op, const llvm::Value* x, const llvm::Value* y) {
    storage_->apply(op, x, y);
}

void State::load(const llvm::Value* x, const llvm::Value* ptr) {
    storage_->load(x, ptr);
}

void State::store(const llvm::Value* ptr, const llvm::Value* x) {
    storage_->store(ptr, x);
}

void State::gep(const llvm::Value* x, const llvm::Value* ptr, const std::vector<const llvm::Value*>& shifts) {
    storage_->gep(x, ptr, shifts);
}

void State::allocate(const llvm::Value* x, const llvm::Value* size) {
    storage_->allocate(x, size);
}

void State::merge(State::Ptr other) {
    this->storage_->joinWith(other->storage_);
}

bool State::empty() const {
    return storage_->empty();
}

std::string State::toString() const {
    return "";
}

bool operator==(const State& lhv, const State& rhv) {
    return lhv.equals(&rhv);
}

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
