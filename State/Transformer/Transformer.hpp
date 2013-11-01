/*
 * Transformer.hpp
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATETRANSFORMER_H_
#define PREDICATETRANSFORMER_H_

#include <llvm/Support/Casting.h>

#include "Factory/Nest.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

#define DELEGATE(CLASS, WHAT) \
    return static_cast<SubClass*>(this)->transform##CLASS(WHAT)

template<class SubClass>
class Transformer {

#define HANDLE_PREDICATE(NAME, CLASS) friend class CLASS;
#include "Predicate/Predicate.def"

#define HANDLE_TERM(NAME, CLASS) friend class CLASS;
#include "Term/Term.def"

protected:

    FactoryNest FN;

public:

    Transformer(FactoryNest FN) : FN(FN) {};

    ////////////////////////////////////////////////////////////////////////////
    //
    // Predicates
    //
    ////////////////////////////////////////////////////////////////////////////

public:

    Predicate::Ptr transform(Predicate::Ptr pred) {
        DELEGATE(Base, pred);
    }

protected:

    Predicate::Ptr transformBase(Predicate::Ptr pred) {
        Predicate::Ptr res;
#define HANDLE_PREDICATE(NAME, CLASS) \
        if (llvm::isa<CLASS>(pred)) { \
            res = static_cast<SubClass*>(this)-> \
                transform##NAME(std::static_pointer_cast<const CLASS>(pred)); \
        }
#include "Predicate/Predicate.def"
        ASSERT(res, "Unsupported predicate type");
        DELEGATE(Predicate, res);
    }

    Predicate::Ptr transformPredicate(Predicate::Ptr p) {
        return p;
    }

#define HANDLE_PREDICATE(NAME, CLASS) \
    typedef std::shared_ptr<const CLASS> CLASS##Ptr; \
    Predicate::Ptr transform##NAME(CLASS##Ptr p) { \
        CLASS##Ptr pp(p->accept(this)); \
        DELEGATE(CLASS, *pp == *p ? p : pp); \
    }
#include "Predicate/Predicate.def"

#define HANDLE_PREDICATE(NAME, CLASS) \
    Predicate::Ptr transform##CLASS(CLASS##Ptr p) { \
        return p; \
    }
#include "Predicate/Predicate.def"

    ////////////////////////////////////////////////////////////////////////////
    //
    // Terms
    //
    ////////////////////////////////////////////////////////////////////////////

public:

    Term::Ptr transform(Term::Ptr term) {
        DELEGATE(Base, term);
    }

protected:

    Term::Ptr transformBase(Term::Ptr term) {
        Term::Ptr res;
#define HANDLE_TERM(NAME, CLASS) \
        if (llvm::isa<CLASS>(term)) { \
            res = static_cast<SubClass*>(this)-> \
                transform##NAME(std::static_pointer_cast<const CLASS>(term)); \
        }
#include "Term/Term.def"
        ASSERT(res, "Unsupported term type");
        DELEGATE(Term, res);
    }

    Term::Ptr transformTerm(Term::Ptr t) {
        return t;
    }

#define HANDLE_TERM(NAME, CLASS) \
    typedef std::shared_ptr<const CLASS> CLASS##Ptr; \
    Term::Ptr transform##NAME(CLASS##Ptr t) { \
        CLASS##Ptr tt = std::static_pointer_cast<const CLASS>(t->accept(this)); \
        DELEGATE(CLASS, tt); \
    }
#include "Term/Term.def"

#define HANDLE_TERM(NAME, CLASS) \
    Term::Ptr transform##CLASS(CLASS##Ptr t) { \
        return t; \
    }
#include "Term/Term.def"

};

#ifdef DELEGATE
#undef DELEGATE
#endif

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* PREDICATETRANSFORMER_H_ */
