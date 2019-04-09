//
// Created by abdullin on 10/18/17.
//

#include "DomainStorage.hpp"
#include "State.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ps {

State::State(const VariableFactory* vf, State::Ptr input) :
        storage_(std::make_shared<DomainStorage>(vf, (input ? input->storage_ : nullptr))) {}

State::State(std::shared_ptr<DomainStorage> storage) :
        storage_(storage) {}

State::State(const State& other) :
        storage_(other.storage_->clone()) {}

bool State::equals(const State* other) const {
    return this->storage_->equals(other->storage_);
}

AbstractDomain::Ptr State::get(Term::Ptr x) const {
    return storage_->get(x);
}

void State::assign(Term::Ptr x, Term::Ptr y) {
    storage_->assign(x, y);
}

void State::assign(Term::Ptr v, AbstractDomain::Ptr domain) {
    storage_->assign(v, domain);
}

void State::apply(llvm::ArithType op, Term::Ptr x, Term::Ptr y, Term::Ptr z) {
    storage_->apply(op, x, y, z);
}

void State::apply(llvm::ConditionType op, Term::Ptr x, Term::Ptr y, Term::Ptr z) {
    storage_->apply(op, x, y, z);
}

void State::apply(CastOperator op, Term::Ptr x, Term::Ptr y) {
    storage_->apply(op, x, y);
}

void State::load(Term::Ptr x, Term::Ptr ptr) {
    storage_->load(x, ptr);
}

void State::store(Term::Ptr ptr, Term::Ptr x) {
    storage_->store(ptr, x);
}

void State::storeWithWidening(Term::Ptr ptr, Term::Ptr x) {
    storage_->storeWithWidening(ptr, x);
}

void State::gep(Term::Ptr x, Term::Ptr ptr, const std::vector<Term::Ptr>& shifts) {
    storage_->gep(x, ptr, shifts);
}

void State::allocate(Term::Ptr x, Term::Ptr size) {
    storage_->allocate(x, size);
}

void State::merge(State::Ptr other) {
    this->storage_->joinWith(other->storage_);
}

bool State::empty() const {
    return storage_->empty();
}

std::string State::toString() const {
    return storage_->toString();
}

bool operator==(const State& lhv, const State& rhv) {
    return lhv.equals(&rhv);
}

std::ostream& operator<<(std::ostream& s, State::Ptr state) {
    s << state->toString();
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, State::Ptr state) {
    s << state->toString();
    return s;
}

}   /* namespace ps */
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"