/*
 * GepTerm.cpp
 *
 *  Created on: Jun 3, 2013
 *      Author: ice-phoenix
 */

#include "Term/GepTerm.h"
#include "Factory/Nest.h"
#include "Term/TermBuilder.h"

#include "State/Transformer/ConstantPropagator.h"

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

Term::Ptr GepTerm::calcShift(const GepTerm *t) {
    FactoryNest FN;
    TermBuilder TB(FN.Term);

    size_t gepBitSize = 64; // FIXME: wat?

    auto&& shift = TB(FN.Term->getIntTerm(0, gepBitSize, llvm::Signedness::Unsigned));

    auto* baseType = llvm::dyn_cast<type::Pointer>(t->getBase()->getType());
    ASSERTC(!!baseType);

    auto&& h = TB(util::head(t->getShifts()));

    auto&& tp = baseType->getPointed();
    auto&& size = TypeUtils::getTypeSizeInElems(tp);

    shift = shift + h * size;

    for (auto&& s : util::tail(t->getShifts())) {
        if (llvm::isa<type::Record>(tp)) {
            auto&& ss = llvm::dyn_cast<OpaqueIntConstantTerm>(s);
            ASSERTC(!!ss);
            auto&& offset = TypeUtils::getStructOffsetInElems(tp, ss->getValue());

            shift = shift + offset;

            tp = GepTerm::getAggregateElement(tp, s);

        } else if (llvm::isa<type::Array>(tp)) {
            tp = GepTerm::getAggregateElement(tp, s);
            size = TypeUtils::getTypeSizeInElems(tp);

            shift = shift + TB(s) * size;

        } else BYE_BYE(Term::Ptr, "Encountered non-aggregate type in GEP: " + TypeUtils::toString(*tp));

    }

    return ConstantPropagator{FN}.transform(shift);
}

} // namespace borealis

#include "Util/unmacros.h"
