//
// Created by abdullin on 10/18/17.
//

#include "State.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ps {

State::State(const State::TermMap& variables, const State::TermMap& constants)
        : variables_(variables), constants_(constants) {}

std::string State::toString() const {
    std::ostringstream ss;
    if (not variables_.empty()) {
        auto&& head = util::head(variables_);
        ss << "  " << head.first->getName() << " = " << head.second->toPrettyString("  ");
        for (auto&& it : util::tail(variables_)) {
            ss << std::endl << "  " << it.first->getName() << " = " << it.second->toPrettyString("  ");
        }
    }
    return ss.str();
}

void State::addVariable(Term::Ptr term, Domain::Ptr domain) {
    variables_[term] = domain;
}

void State::addConstant(Term::Ptr term, Domain::Ptr domain) {
    constants_[term] = domain;
}

Domain::Ptr State::find(Term::Ptr term) const {
    auto var_find = findVariable(term);
    return var_find ? var_find : findConstant(term);
}

void State::merge(State::Ptr other) {
    mergeConstants(other);
    mergeTerms(other);
}

const State::TermMap& State::getVariables() {
    return variables_;
}

const State::TermMap& State::getConstants() {
    return constants_;
}

void State::mergeConstants(State::Ptr other) {
    for (auto&& it : other->getConstants()) {
        auto itl = constants_.find(it.first);
        if (itl == constants_.end()) {
            constants_.insert(it);
        } else {
            ASSERT(itl->second->equals(it.second.get()), "constant domain changed");
        }
    }
}

void State::mergeTerms(State::Ptr other) {
    for (auto&& it : other->getVariables()) {
        auto itl = variables_.find(it.first);
        if (itl == variables_.end()) {
            variables_.insert(it);
        } else {
            variables_[it.first] = itl->second->join(it.second);
        }
    }
}

Domain::Ptr State::findConstant(Term::Ptr term) const {
    auto it = constants_.find(term);
    return it == constants_.end() ? nullptr : it->second;
}

Domain::Ptr State::findVariable(Term::Ptr term) const {
    auto it = variables_.find(term);
    return it == variables_.end() ? nullptr : it->second;
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