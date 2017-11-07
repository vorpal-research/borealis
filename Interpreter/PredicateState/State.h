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

struct TermHashWType {
    size_t operator()(Term::Ptr term) const noexcept {
        return util::hash::defaultHasher()(term, term->getType());
    }
};

struct TermEqualsWType {
    bool operator()(Term::Ptr lhv, Term::Ptr rhv) const noexcept {
        return lhv->equals(rhv.get()) && lhv->getType() == rhv->getType();
    }
};

class State: public std::enable_shared_from_this<State> {
public:

    using Ptr = std::shared_ptr<State>;
    using TermMap = std::unordered_map<Term::Ptr, Domain::Ptr, TermHashWType, TermEqualsWType>;

    State() = default;
    State(const TermMap& variables, const TermMap& constants);

    std::string toString() const;
    const TermMap& getVariables();
    const TermMap& getConstants();
    void addVariable(Term::Ptr term, Domain::Ptr domain);
    void addConstant(Term::Ptr term, Domain::Ptr domain);
    Domain::Ptr find(Term::Ptr term) const;

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
