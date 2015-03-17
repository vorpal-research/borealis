/*
 * OpaqueMemberAccessTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/OpaqueMemberAccessTerm.h"

namespace borealis {

OpaqueMemberAccessTerm::OpaqueMemberAccessTerm(
        Type::Ptr type,
        Term::Ptr lhv,
        const std::string& property,
        bool indirect):
    Term(
        class_tag(*this),
        type,
        lhv->getName() + (indirect ? "->" : ".") + property
    ), property(property), indirect(indirect) {
    subterms = { lhv };
};

Term::Ptr OpaqueMemberAccessTerm::getLhv() const {
    return subterms[0];
}

const std::string& OpaqueMemberAccessTerm::getProperty() const {
    return property;
}

bool OpaqueMemberAccessTerm::isIndirect() const {
    return indirect;
}

bool OpaqueMemberAccessTerm::equals(const Term* other) const {
    if (auto* that = llvm::dyn_cast_or_null<Self>(other)) {
        return Term::equals(other) &&
                that->property == property &&
                that->indirect == indirect;
    } else return false;
}

size_t OpaqueMemberAccessTerm::hashCode() const {
    return util::hash::defaultHasher()(Term::hashCode(), property, indirect);
}

} // namespace borealis
