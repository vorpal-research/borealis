/*
 * VarArgumentTerm.cpp
 *
 *  Created on: Oct 30, 2015
 *      Author: belyaev
 */

#include "Term/VarArgumentTerm.h"

namespace borealis {

VarArgumentTerm::VarArgumentTerm(Type::Ptr type, unsigned int idx):
    Term(
        class_tag(*this),
        type,
        tfm::format("<vararg%d>", idx)
    ), idx(idx) {}

unsigned VarArgumentTerm::getIdx() const {
    return idx;
}


bool VarArgumentTerm::equals(const Term* other) const {
    if (auto* that = llvm::dyn_cast_or_null<Self>(other)) {
        return Term::equals(other) &&
                that->idx == idx;
    } else return false;
}

size_t VarArgumentTerm::hashCode() const {
    return util::hash::defaultHasher()(Term::hashCode(), idx);
}

} // namespace borealis
