/*
 * ArgumentTerm.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include "Term/ArgumentTerm.h"

namespace borealis {

ArgumentTerm::ArgumentTerm(Type::Ptr type, unsigned int idx, const std::string& name, ArgumentKind kind):
    Term(
        class_tag(*this),
        type,
        name
    ), idx(idx), kind(kind) {}

unsigned ArgumentTerm::getIdx() const {
    return idx;
}

ArgumentKind ArgumentTerm::getKind() const {
    return kind;
}

bool ArgumentTerm::equals(const Term* other) const {
    if (auto* that = llvm::dyn_cast_or_null<Self>(other)) {
        return Term::equals(other) &&
                that->idx == idx &&
                that->kind == kind;
    } else return false;
}

size_t ArgumentTerm::hashCode() const {
    return util::hash::defaultHasher()(Term::hashCode(), idx, kind);
}

} // namespace borealis
