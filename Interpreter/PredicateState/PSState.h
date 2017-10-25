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
class PSState: public std::enable_shared_from_this<PSState> {
public:

    using Ptr = std::shared_ptr<PSState>;
    using TermMap = std::unordered_map<Term::Ptr, Domain::Ptr, TermHashWType, TermEqualsWType>;

    PSState() = default;


    std::string toString() const;
    const TermMap& getVariables();
    void addTerm(Term::Ptr term, Domain::Ptr domain);
    void addConstant(Term::Ptr term, Domain::Ptr domain);
    Domain::Ptr find(Term::Ptr term) const;

    void merge(PSState::Ptr other);

private:
    TermMap terms_;
    TermMap constants_;
};

std::ostream& operator<<(std::ostream& s, PSState::Ptr state);
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, PSState::Ptr state);

}   /* namespace absint */
}   /* namespace borealis */

#endif //BOREALIS_PSSTATE_H
