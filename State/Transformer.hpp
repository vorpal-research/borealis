/*
 * Transformer.hpp
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATETRANSFORMER_H_
#define PREDICATETRANSFORMER_H_

#include <llvm/Support/Casting.h>

#include "Predicate/PredicateFactory.h"
#include "Term/TermFactory.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

#define HANDLE_PREDICATE(NAME, CLASS) class CLASS;
#include "Predicate/Predicate.def"

#define HANDLE_TERM(NAME, CLASS) class CLASS;
#include "Term/Term.def"

#define DELEGATE(CLASS, WHAT) \
    return static_cast<SubClass*>(this)->transform##CLASS(WHAT);

template<class SubClass>
class Transformer {

    ////////////////////////////////////////////////////////////////////////////
    //
    // Predicates
    //
    ////////////////////////////////////////////////////////////////////////////

public:

    Predicate::Ptr transform(Predicate::Ptr pred) {
#define HANDLE_PREDICATE(NAME, CLASS) \
        if (llvm::isa<CLASS>(pred)) { \
            return static_cast<SubClass*>(this)-> \
                transform##NAME(std::static_pointer_cast<const CLASS>(pred)); \
        }
#include "Predicate/Predicate.def"
        BYE_BYE(Predicate::Ptr, "Unsupported predicate type");
    }

protected:

#define HANDLE_PREDICATE(NAME, CLASS) \
    typedef std::shared_ptr<const CLASS> CLASS##Ptr; \
    Predicate::Ptr transform##NAME(CLASS##Ptr p) { \
        CLASS##Ptr pp(p->accept(this)); \
        DELEGATE(CLASS, pp); \
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
#define HANDLE_TERM(NAME, CLASS) \
        if (llvm::isa<CLASS>(term)) { \
            return static_cast<SubClass*>(this)-> \
                transform##NAME(std::static_pointer_cast<const CLASS>(term)); \
        }
#include "Term/Term.def"
        BYE_BYE(Term::Ptr, "Unsupported term type");
    }

protected:

#define HANDLE_TERM(NAME, CLASS) \
    typedef std::shared_ptr<const CLASS> CLASS##Ptr; \
    Term::Ptr transform##NAME(CLASS##Ptr t) { \
        CLASS##Ptr tt(t->accept(this)); \
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
