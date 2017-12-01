//
// Created by abdullin on 10/18/17.
//

#ifndef BOREALIS_PSSTATE_H
#define BOREALIS_PSSTATE_H

#include <memory>

#include "Interpreter/Domain/Domain.h"
#include "Logging/logstream.hpp"
#include "Term/Term.h"
#include "Util/hash.hpp"

namespace borealis {
namespace absint {
namespace ps {

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
    using TermMap = std::unordered_map<Term::Ptr, Domain::Ptr, TermHash, TermEqualsWType>;

    State() = default;
    State(const TermMap& variables, const TermMap& constants);

    std::string toString() const;
    bool empty() const;
    const TermMap& getVariables();
    const TermMap& getConstants();
    void addVariable(Term::Ptr term, Domain::Ptr domain);
    void addConstant(Term::Ptr term, Domain::Ptr domain);
    Domain::Ptr find(Term::Ptr term) const;
    Domain::Ptr findConstant(Term::Ptr term) const;
    Domain::Ptr findVariable(Term::Ptr term) const;

    void merge(State::Ptr other);

private:

    void mergeConstants(State::Ptr other);
    void mergeTerms(State::Ptr other);

    TermMap variables_;
    TermMap constants_;
};

std::ostream& operator<<(std::ostream& s, State::Ptr state);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, State::Ptr state);

}   /* namespace ps */
}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_PSSTATE_H
