#ifndef TERM_REMAPPER_HPP
#define TERM_REMAPPER_HPP

#include "Term/TermUtils.hpp"
#include "State/Transformer/CachingTransformer.hpp"
#include "State/Transformer/ConstantPropagator.h"

namespace borealis {

template<class Mapping>
class TermEvaluator : public borealis::CachingTransformer<TermEvaluator> {
    Mapping mapping;
    using ValueMap = std::unordered_map<Term::Ptr, Term::Ptr, TermHash, TermEquals>;
    ValueMap values;
    using MemorySpace = std::unordered_map<size_t, Term::Ptr>;
    using Memory = std::unordered_map<size_t, MemorySpace>;
    Memory memory;
    Memory bounds;
    std::unordered_map<std::string, Memory> properties;
    ConstantPropagator CP{ FN };

    template<class T>
    T eval(llvm::ArithType opcode, T lhv, T rhv) {
        switch(opcode) {
            case llvm::ArithType::ADD: lhv + rhv;
            case llvm::ArithType::SUB: lhv - rhv;
        }
    }

public:
    struct InvalidMapping: std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct Chop: std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    using borealis::CachingTransformer::CachingTransformer;

    Term::Ptr transformTerm(Term::Ptr input) {
        if(TermUtils::isNamedTerm(input)){
            return values[input] = mapping(input);
        } else if(not TermUtils::isConstantTerm(input)) {
            return CP.transform(input);
        }
        return input;
    }

    Term::Ptr transformAxiomTerm(AxiomTermPtr axiom) {
        if(auto ax = llvm::dyn_cast<OpaqueBoolConstantTerm>(axiom->getRhv())) {
            if(ax->getValue() != true) throw InvalidMapping("Axiom failed");
        } else throw InvalidMapping("Axiom not a boolean constant");

        return axiom->getLhv();
    }

    Term::Ptr transformLoadTerm(LoadTermPtr load) {
        if(auto&& ptr = llvm::dyn_cast<type::Pointer>(load->getRhv()->getType())) {
            auto mspace = ptr->getMemspace();
            if(auto&& addr = TermUtils::getIntegerValue(load->getRhv())) {
                auto&& mem = memory[mspace];
                auto it = memory[mspace].find(addr.getUnsafe());
                if(it != mem.end()) {
                    return it->second;
                }
                return mapping(load);
            }
        }
        UNREACHABLE("Loading from non-pointer");
    }

    Term::Ptr transformGepTerm(GepTermPtr gep) {
        auto shift = GepTerm::calcShift(gep.get());
        return CP.transform(FN.Term->getBinaryTerm(llvm::ArithType::ADD, gep->getBase(), shift));
    }

    Predicate::Ptr transformEquality(EqualityPredicatePtr eq) {
        switch(eq->getType()) {
            case PredicateType::STATE:
            case PredicateType::INVARIANT: {
                auto reValue = transform(eq->getRhv());
                values[eq->getLhv()] = reValue;

            }
            default: {
                auto lhvValue = transform(eq->getLhv());
                auto rhvValue = transform(eq->getRhv());
                if(not TermEquals{}(lhvValue, rhvValue)) throw Chop{"Chop chop chop"};
            }
        }
        return eq;
    }

    Predicate::Ptr transformInequality(InequalityPredicatePtr eq) {
        switch(eq->getType()) {
            case PredicateType::STATE:
            case PredicateType::INVARIANT: {
                throw std::runtime_error("State inequality predicate detected");
            }
            default: {
                auto lhvValue = transform(eq->getLhv());
                auto rhvValue = transform(eq->getRhv());
                if(TermEquals{}(lhvValue, rhvValue)) throw Chop{"Chop chop chop"};
            }
        }
        return eq;
    }

    Predicate::Ptr transformStore(StorePredicatePtr store) {
        auto rhv = transform(store->getRhv());
        auto lhv = transform(store->getLhv());
        if(auto&& ptr = llvm::dyn_cast<type::Pointer>(lhv->getType())) {
            auto mspace = ptr->getMemspace();
            if(auto&& addr = TermUtils::getIntegerValue(lhv)) {
                memory[mspace][addr.getUnsafe()] = rhv;
            }
        }
    }

};

} /* namespace borealis */

#endif // TERM_REMAPPER_HPP
