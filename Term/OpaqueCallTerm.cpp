/*
 * OpaqueCallTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/OpaqueCallTerm.h"

namespace borealis {

OpaqueCallTerm::OpaqueCallTerm(Type::Ptr type, Term::Ptr lhv, const std::vector<Term::Ptr>& rhv):
    Term(
        class_tag(*this),
        type,
        lhv->getName() + "(" +
            util::viewContainer(rhv)
            .map([](auto&& trm) { return trm->getName(); })
            .reduce("", [](auto&& acc, auto&& e) { return acc + ", " + e; }) +
        ")"
    ) {
    subterms.insert(subterms.end(), lhv);
    subterms.insert(subterms.end(), rhv.begin(), rhv.end());
};

Term::Ptr OpaqueCallTerm::getLhv() const {
    return subterms[0];
}
auto OpaqueCallTerm::getRhv() const -> decltype(util::viewContainer(subterms)) {
    return util::viewContainer(subterms).drop(1);
}

} // namespace borealis
