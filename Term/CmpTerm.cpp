/*
 * CmpTerm.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#include "Term/CmpTerm.h"

namespace borealis {

CmpTerm::CmpTerm(Type::Ptr type, llvm::ConditionType opcode, Term::Ptr lhv, Term::Ptr rhv):
    Term(
        class_tag(*this),
        type,
        "(" + lhv->getName() + " " + llvm::conditionString(opcode) + " " + rhv->getName() + ")"
    ), opcode(opcode) {
    subterms = { lhv, rhv };
};

llvm::ConditionType CmpTerm::getOpcode() const {
    return opcode;
}

Term::Ptr CmpTerm::getLhv() const {
    return subterms[0];
}

Term::Ptr CmpTerm::getRhv() const {
    return subterms[1];
}

bool CmpTerm::equals(const Term* other) const {
    if (auto* that = llvm::dyn_cast_or_null<Self>(other)) {
        return Term::equals(other) &&
                that->opcode == opcode;
    } else return false;
}

size_t CmpTerm::hashCode() const {
    return util::hash::defaultHasher()(Term::hashCode(), opcode);
}

} // namespace borealis
