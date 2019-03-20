//
// Created by abdullin on 10/18/17.
//

#ifndef BOREALIS_PSSTATE_H
#define BOREALIS_PSSTATE_H

#include <memory>

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Logging/logstream.hpp"
#include "Term/Term.h"
#include "Util/hash.hpp"

namespace borealis {
namespace absint {

class VariableFactory;

namespace ps {

class DomainStorage;

struct TermEqualsWType {
    bool operator()(Term::Ptr lhv, Term::Ptr rhv) const noexcept {
        // This is generally fucked up
        if (lhv->equals(rhv.get())) {
            if (lhv->getType() == rhv->getType()) return true;
            auto&& t1 = llvm::dyn_cast<type::Integer>(lhv->getType().get());
            auto&& t2 = llvm::dyn_cast<type::Integer>(rhv->getType().get());
            if (t1 && t2) return t1->getBitsize() == t2->getBitsize();
            else return false;
        }
        return false;
    }
};

class State: public std::enable_shared_from_this<State> {
public:

    using Ptr = std::shared_ptr<State>;
    using TermMap = std::unordered_map<Term::Ptr, AbstractDomain::Ptr, TermHash, TermEqualsWType>;

    explicit State(const VariableFactory* vf);
    explicit State(std::shared_ptr<DomainStorage> storage);
    State(const State& other);

    bool equals(const State* other) const;
    friend bool operator==(const State& lhv, const State& rhv);

    AbstractDomain::Ptr get(Term::Ptr x) const;
    void assign(Term::Ptr x, Term::Ptr y);
    void assign(Term::Ptr v, AbstractDomain::Ptr domain);
    void apply(llvm::ArithType op, Term::Ptr x, Term::Ptr y, Term::Ptr z);
    void apply(llvm::ConditionType op, Term::Ptr x, Term::Ptr y, Term::Ptr z);
    void apply(CastOperator op, Term::Ptr x, Term::Ptr y);
    void load(Term::Ptr x, Term::Ptr ptr);
    void store(Term::Ptr ptr, Term::Ptr x);
    void storeWithWidening(Term::Ptr ptr, Term::Ptr x);
    void gep(Term::Ptr x, Term::Ptr ptr, const std::vector<Term::Ptr>& shifts);

    void allocate(Term::Ptr x, Term::Ptr size);

    void merge(State::Ptr other);

    bool empty() const;
    std::string toString() const;

    std::pair<State::Ptr, State::Ptr> split(Term::Ptr condition) const;

private:

    std::shared_ptr<DomainStorage> storage_;
};

std::ostream& operator<<(std::ostream& s, State::Ptr state);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, State::Ptr state);

}   /* namespace ps */
}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_PSSTATE_H
