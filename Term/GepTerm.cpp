/*
 * GepTerm.cpp
 *
 *  Created on: Jun 3, 2013
 *      Author: ice-phoenix
 */

#include "Term/GepTerm.h"

#include "Util/macros.h"

namespace borealis {

GepTerm::GepTerm(Type::Ptr type, Term::Ptr base, const std::vector<Term::Ptr>& shifts):
    GepTerm(type, base, shifts, false) {};

GepTerm::GepTerm(Type::Ptr type, Term::Ptr base, const std::vector<Term::Ptr>& shifts, bool inBounds):
    Term(
        class_tag(*this),
        type,
        std::string("gep") +
           (inBounds? "[inbounds]" : "") +
           "(" + base->getName() + "," +
           util::viewContainer(shifts)
                .map([](auto&& s) { return s->getName(); })
                .fold(std::string("0"), [](auto&& acc, auto&& e) { return acc + "+" + e; }) +
           ")"
    ), isTriviallyInbounds_(inBounds) {
    subterms.insert(subterms.end(), base);
    subterms.insert(subterms.end(), shifts.begin(), shifts.end());
};

Term::Ptr GepTerm::getBase() const {
    return subterms[0];
}

auto GepTerm::getShifts() const -> decltype(util::viewContainer(subterms)) {
    return util::viewContainer(subterms).drop(1);
}

bool GepTerm::isTriviallyInbounds() const {
    return isTriviallyInbounds_;
}

Type::Ptr GepTerm::getAggregateElement(Type::Ptr parent, Term::Ptr idx) {
    if (auto* structType = llvm::dyn_cast<type::Record>(parent)) {
        auto&& body = structType->getBody()->get();
        auto&& cIdx = llvm::dyn_cast<OpaqueIntConstantTerm>(idx);
        ASSERTC(!!cIdx);
        auto&& index = cIdx->getValue();
        ASSERTC(index >= 0);
        auto&& uIndex = (unsigned long long) index;
        ASSERTC(uIndex < body.getNumFields());
        return body.at(uIndex).getType();
    } else if (auto* arrayType = llvm::dyn_cast<type::Array>(parent)) {
        return arrayType->getElement();
    }

    BYE_BYE(Type::Ptr, "getAggregateElement on non-aggregate type: " + TypeUtils::toString(*parent));
}

Type::Ptr GepTerm::getGepChild(Type::Ptr parent, const std::vector<Term::Ptr>& index) {
    auto&& ptrType = llvm::dyn_cast<type::Pointer>(parent);
    ASSERT(!!ptrType, "getGepChild argument is not a pointer");
    auto&& ret = ptrType->getPointed();
    for (auto&& ix : util::tail(index)) {
        ret = getAggregateElement(ret, ix);
    }
    return ret;
}

} // namespace borealis

#include "Util/unmacros.h"
