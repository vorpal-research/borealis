//
// Created by abdullin on 10/18/17.
//

#include "PSState.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

std::string PSState::toString() const {
    std::ostringstream ss;
    if (not terms_.empty()) {
        auto&& head = util::head(terms_);
        ss << "  " << head.first->getName() << " = " << head.second->toPrettyString("  ");
        for (auto&& it : util::tail(terms_)) {
            ss << std::endl << "  " << it.first->getName() << " = " << it.second->toPrettyString("  ");
        }
    }
    return ss.str();
}

void PSState::addTerm(Term::Ptr term, Domain::Ptr domain) {
    terms_[term] = domain;
}

void PSState::addConstant(Term::Ptr term, Domain::Ptr domain) {
    constants_[term] = domain;
}

Domain::Ptr PSState::find(Term::Ptr term) const {
    auto it = terms_.find(term);
    if (it != terms_.end()) return it->second;
    auto it2 = constants_.find(term);
    return it2 == constants_.end() ? nullptr : it2->second;
}

void PSState::merge(PSState::Ptr other) {
    mergeConstants(other);
    mergeTerms(other);
}

const PSState::TermMap& PSState::getVariables() {
    return terms_;
}

const PSState::TermMap& PSState::getConstants() {
    return constants_;
}

void PSState::mergeConstants(PSState::Ptr other) {
    for (auto&& it : other->getConstants()) {
        auto itl = constants_.find(it.first);
        if (itl == constants_.end()) {
            constants_.insert(it);
        } else {
            ASSERT(itl->second->equals(it.second.get()), "constant domain changed");
        }
    }
}

void PSState::mergeTerms(PSState::Ptr other) {
    for (auto&& it : other->getVariables()) {
        auto itl = terms_.find(it.first);
        if (itl == terms_.end()) {
            terms_.insert(it);
        } else {
            terms_[it.first] = itl->second->join(it.second);
        }
    }
}

std::ostream& operator<<(std::ostream& s, PSState::Ptr state) {
    s << state->toString();
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, PSState::Ptr state) {
    s << state->toString();
    return s;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"