//
// Created by belyaev on 6/28/16.
//

#ifndef TERMUTILS_HPP
#define TERMUTILS_HPP

#include <State/Transformer/TermCollector.h>
#include "Term/TermFactory.h"

namespace borealis {

struct TermUtils {

    static Term::Ptr stripCasts(Term::Ptr t) {
        if(auto&& cast = llvm::dyn_cast<CastTerm>(t)) {
            return stripCasts(cast->getRhv());
        }
        if(auto&& ax = llvm::dyn_cast<AxiomTerm>(t)) {
            return stripCasts(ax->getRhv());
        }
        return t;
    }

    static bool isNamedTerm(Term::Ptr t) {
        if(not t) return false;

        return llvm::is_one_of<
                   ValueTerm,
                   ArgumentTerm,
                   VarArgumentTerm,
                   ConstTerm,
                   FreeVarTerm,
                   OpaqueVarTerm,
                   ReturnValueTerm,
                   ReturnPtrTerm
               >(t);
    }

    static bool isConstantTerm(Term::Ptr t) {
        if(not t) return false;

        return llvm::is_one_of<
                   OpaqueIntConstantTerm,
                   OpaqueBigIntConstantTerm,
                   OpaqueBoolConstantTerm,
                   OpaqueFloatingConstantTerm,
                   OpaqueNullPtrTerm,
                   OpaqueInvalidPtrTerm,
                   OpaqueUndefTerm, //???
                   OpaqueStringConstantTerm //???
               >(t);
    }

    static util::option<std::string> getStringValue(Term::Ptr t) {
        if(not t) return util::nothing();

        if(auto&& bc = llvm::dyn_cast<OpaqueStringConstantTerm>(stripCasts(t))) {
            return util::just(bc->getValue());
        }
        return util::nothing();
    }

    static util::option<bool> getBoolValue(Term::Ptr t) {
        if(not t) return util::nothing();

        if(auto&& bc = llvm::dyn_cast<OpaqueBoolConstantTerm>(stripCasts(t))) {
            return util::just(bc->getValue());
        }
        return util::nothing();
    }

    static util::option<int64_t> getIntegerValue(Term::Ptr t) {
        if(not t) return util::nothing();

        if(auto&& bc = llvm::dyn_cast<OpaqueIntConstantTerm>(stripCasts(t))) {
            return util::just(bc->getValue());
        }
        return util::nothing();
    }

    static util::option<uint64_t> getUIntegerValue(Term::Ptr t) {
        if(not t) return util::nothing();

        if(auto&& bc = llvm::dyn_cast<OpaqueIntConstantTerm>(stripCasts(t))) {
            return util::just(static_cast<uint64_t >(bc->getValue()));
        }
        return util::nothing();
    }

    static util::option<std::string> getIntegerValueRep(Term::Ptr t) {
        if(not t) return util::nothing();

        t = stripCasts(t);

        if(auto&& bc = llvm::dyn_cast<OpaqueIntConstantTerm>(t)) {
            return util::just(util::toString(bc->getValue()));
        } else if(auto&& bc = llvm::dyn_cast<OpaqueBigIntConstantTerm>(t)) {
            return util::just(bc->getRepresentation());
        }
        return util::nothing();
    }

    static util::option<std::string> getUIntegerValueRep(Term::Ptr t) {
        if(not t) return util::nothing();

        t = stripCasts(t);

        if(auto&& bc = llvm::dyn_cast<OpaqueIntConstantTerm>(t)) {
            return util::just(util::toString(static_cast<uint64_t >(bc->getValue())));
        } else if(auto&& bc = llvm::dyn_cast<OpaqueBigIntConstantTerm>(t)) {
            return util::just(bc->getRepresentation());
        }
        return util::nothing();
    }

    template<class Transformable>
    static std::unordered_set<Term::Ptr, TermHash, TermEquals> getFullTermSet(Transformable tr) {
        TermCollector<> TC{ FactoryNest() };
        TC.transform(tr);
        return TC.moveTerms();
    }

};

} /* namespace borealis */

#endif //TERMUTILS_HPP
