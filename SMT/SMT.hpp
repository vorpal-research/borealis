/*
 * SMT.hpp
 *
 *  Created on: Jul 18, 2013
 *      Author: ice-phoenix
 */

#ifndef BOREALIS_SMT_H_
#define BOREALIS_SMT_H_

#include "SMT/SMTUtil.h"

#include "Predicate/Predicate.h"
#include "Predicate/Predicate.def"
#include "State/PredicateState.h"
#include "State/PredicateState.def"
#include "Term/Term.h"
#include "Term/Term.def"

#include "Util/macros.h"

namespace borealis {

template<class Impl>
struct SMT {

    static Bool<Impl> doit(
            Predicate::Ptr p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
#define HANDLE_PREDICATE(NAME, CLASS) \
        if (auto* pp = llvm::dyn_cast<CLASS>(p)) { \
            return SMTImpl<Impl, CLASS>::doit(pp, ef, ctx); \
        }
#include "Predicate/Predicate.def"
        BYE_BYE(Bool<Impl>, "Should never happen!");
    }


    static Bool<Impl> doit(
            PredicateState::Ptr s,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
#define HANDLE_STATE(NAME, CLASS) \
        if (auto* ss = llvm::dyn_cast<CLASS>(s)) { \
            return SMTImpl<Impl, CLASS>::doit(ss, ef, ctx); \
        }
#include "State/PredicateState.def"
        BYE_BYE(Bool<Impl>, "Should never happen!");
    }


    static Dynamic<Impl> doit(
            Term::Ptr t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
#define HANDLE_TERM(NAME, CLASS) \
        if (auto* tt = llvm::dyn_cast<CLASS>(t)) { \
            return SMTImpl<Impl, CLASS>::doit(tt, ef, ctx); \
        }
#include "Term/Term.def"
        BYE_BYE(Dynamic<Impl>, "Should never happen!");
    }

};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* BOREALIS_SMT_H_ */
