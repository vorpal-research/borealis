/*
 * Transformer.h
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATETRANSFORMER_H_
#define PREDICATETRANSFORMER_H_

#include <llvm/Support/Casting.h>

#include "Util/util.h"

namespace borealis {



#define HANDLE_PREDICATE(NAME, CLASS) class CLASS;
#include "Predicate/Predicate.def"

#define HANDLE_TERM(NAME, CLASS) class CLASS;
#include "Term/Term.def"

#define DELEGATE(CLASS, WHAT) \
    return static_cast<SubClass*>(this)-> \
        transform##CLASS(static_cast<const CLASS*>(WHAT))



template<class SubClass>
class Transformer {

    ////////////////////////////////////////////////////////////////////////////
    //
    // Predicates
    //
    ////////////////////////////////////////////////////////////////////////////

public:

    Predicate::Ptr transform(Predicate::Ptr pred) {
        return Predicate::Ptr(transform(pred.get()));
    }

protected:

    const Predicate* transform(const Predicate* pred) {
#define HANDLE_PREDICATE(NAME, CLASS) \
        if (llvm::isa<CLASS>(pred)) { \
            return static_cast<SubClass*>(this)-> \
                transform##NAME(static_cast<const CLASS*>(pred)); \
        }
#include "Predicate/Predicate.def"
        return borealis::util::sayonara<const Predicate*>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                "Unsupported predicate type");
    }

#define HANDLE_PREDICATE(NAME, CLASS) \
    const Predicate* transform##NAME(const CLASS* p) { \
        const CLASS* pp = p->accept(this); \
        DELEGATE(CLASS, pp); \
    }
#include "Predicate/Predicate.def"

#define HANDLE_PREDICATE(NAME, CLASS) \
    const Predicate* transform##CLASS(const CLASS* p) { \
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
        return Term::Ptr(transform(term.get()));
    }

protected:

    const Term* transform(const Term* term) {
#define HANDLE_TERM(NAME, CLASS) \
        if (llvm::isa<CLASS>(term)) { \
            return static_cast<SubClass*>(this)-> \
                transform##NAME(static_cast<const CLASS*>(term)); \
        }
#include "Term/Term.def"
        return borealis::util::sayonara<const Term*>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                "Unsupported term type");
    }

#define HANDLE_TERM(NAME, CLASS) \
    const Term* transform##NAME(const CLASS* t) { \
        DELEGATE(CLASS, t); \
    }
#include "Term/Term.def"

#define HANDLE_TERM(NAME, CLASS) \
    const Term* transform##CLASS(const CLASS* /* t */) { \
        return borealis::util::sayonara<const Term*>(__FILE__, __LINE__, __PRETTY_FUNCTION__, \
            "transform" #CLASS " has to be overridden"); \
    }
#include "Term/Term.def"

    ////////////////////////////////////////////////////////////////////////////
    //
    // Friends
    //
    ////////////////////////////////////////////////////////////////////////////

#define HANDLE_PREDICATE(NAME, CLASS) friend class CLASS;
#include "Predicate/Predicate.def"

};

#ifdef DELEGATE
#undef DELEGATE
#endif

} /* namespace borealis */

#endif /* PREDICATETRANSFORMER_H_ */
