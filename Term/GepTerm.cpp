/*
 * GepTerm.cpp
 *
 *  Created on: Jun 3, 2013
 *      Author: ice-phoenix
 */

#include "Term/GepTerm.h"

namespace borealis {

GepTerm::GepTerm(Type::Ptr type, Term::Ptr base, const std::vector<Term::Ptr>& shifts):
    Term(
        class_tag(*this),
        type,
        "gep(" + base->getName() + "," +
            util::viewContainer(shifts)
            .map([](auto&& s) { return s->getName(); })
            .fold(std::string("0"), [](auto&& acc, auto&& e) { return acc + "+" + e; }) +
        ")"
    ) {
    subterms.insert(subterms.end(), base);
    subterms.insert(subterms.end(), shifts.begin(), shifts.end());
};

Term::Ptr GepTerm::getBase() const {
    return subterms[0];
}

auto GepTerm::getShifts() const -> decltype(util::viewContainer(subterms)) {
    return util::viewContainer(subterms).drop(1);
}

} // namespace borealis
