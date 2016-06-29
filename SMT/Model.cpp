//
// Created by belyaev on 6/28/16.
//

#include "SMT/Model.h"

#include "Term/TermUtils.hpp"

#include "Util/macros.h"

namespace borealis {
namespace smt {

static Term::Ptr getOrUndef(const FactoryNest& FN, const Model::assignments_t& map, Term::Ptr key) {
    for(auto&& res : util::at(map, key)) {
        return res;
    }

    return nullptr;
}

Term::Ptr Model::query(Term::Ptr t) const {
    std::cerr << *this << std::endl;
    std::cerr << tfm::format("query(%s)", t) << std::endl;

    if(TermUtils::isConstantTerm(t)) return t;

    if(TermUtils::isNamedTerm(t)) {
        return getOrUndef(FN, assignments, t);
    }

    if(auto&& cast = llvm::dyn_cast<CastTerm>(t)) {
        auto&& resolve = query(cast->getRhv());
        return FN.Term->getCastTerm(cast->getType(), cast->isSignExtend(), resolve);
    }

    if(auto&& load = llvm::dyn_cast<LoadTerm>(t)) {
        auto&& ptr = load->getRhv();
        auto&& reptr = TermUtils::stripCasts(query(ptr));

        size_t memspace = 0;
        if(auto&& ptrt = llvm::dyn_cast<type::Pointer>(reptr->getType())) {
            memspace = ptrt->getMemspace();
        }

        if(not memories.count(memspace)) return nullptr;

        auto&& targetMem = memories.at(memspace).getFinalMemoryShape();

        return getOrUndef(FN, targetMem, reptr);
    }

    if(auto&& bd = llvm::dyn_cast<BoundTerm>(t)) {
        auto&& ptr = TermUtils::stripCasts(bd->getRhv());
        auto&& reptr = TermUtils::stripCasts(query(ptr));

        size_t memspace = 0;
        if(auto&& ptrt = llvm::dyn_cast<type::Pointer>(reptr->getType())) {
            memspace = ptrt->getMemspace();
        }

        if(not memories.count(memspace)) return nullptr;

        auto&& targetMem = bounds.at(memspace).getFinalMemoryShape();

        return getOrUndef(FN, targetMem, reptr);
    }

    if(auto&& rp = llvm::dyn_cast<ReadPropertyTerm>(t)) {
        auto&& ptr = TermUtils::stripCasts(rp->getRhv());
        auto&& prop = rp->getPropertyName()->getName();

        auto&& reptr = TermUtils::stripCasts(query(ptr));

        if(not properties.count(prop)) return nullptr;

        auto&& targetMem = properties.at(prop).getFinalMemoryShape();
        return getOrUndef(FN, targetMem, reptr);
    }


    UNREACHABLE(tfm::format("Illegal term: %s", t));

}

} /* namespace smt */
} /* namespace borealis */

#include "Util/unmacros.h"
