/*
 * UnaryTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/UnaryTerm.h"

namespace borealis {

UnaryTerm::UnaryTerm(Type::Ptr type, llvm::UnaryArithType opcode, Term::Ptr rhv):
    Term(
        class_tag(*this),
        type,
        llvm::unaryArithString(opcode) + "(" + rhv->getName() + ")"
    ), opcode(opcode) {
    subterms = { rhv };
};


llvm::UnaryArithType UnaryTerm::getOpcode() const {
    return opcode;
}

Term::Ptr UnaryTerm::getRhv() const {
    return subterms[0];
}

bool UnaryTerm::equals(const Term* other) const {
    if (auto* that = llvm::dyn_cast_or_null<Self>(other)) {
        return Term::equals(other) &&
                that->opcode == opcode;
    } else return false;
}

size_t UnaryTerm::hashCode() const {
    return util::hash::defaultHasher()(Term::hashCode(), opcode);
}

} // namespace borealis
