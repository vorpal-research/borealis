/*
 * BinaryTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/BinaryTerm.h"

namespace borealis {

BinaryTerm::BinaryTerm(Type::Ptr type, llvm::ArithType opcode, Term::Ptr lhv, Term::Ptr rhv):
    Term(
        class_tag(*this),
        type,
        "(" + lhv->getName() + " " + llvm::arithString(opcode) + " " + rhv->getName() + ")"
    ), opcode(opcode) {
    subterms = { lhv, rhv };
};

llvm::ArithType BinaryTerm::getOpcode() const {
    return opcode;
}

Term::Ptr BinaryTerm::getLhv() const {
    return subterms[0];
}

Term::Ptr BinaryTerm::getRhv() const {
    return subterms[1];
}

bool BinaryTerm::equals(const Term* other) const {
    if (auto* that = llvm::dyn_cast_or_null<Self>(other)) {
        return Term::equals(other) &&
                that->opcode == opcode;
    } else return false;
}

size_t BinaryTerm::hashCode() const {
    return util::hash::defaultHasher()(Term::hashCode(), opcode);
}

} // namespace borealis
