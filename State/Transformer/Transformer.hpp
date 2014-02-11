/*
 * Transformer.hpp
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATETRANSFORMER_H_
#define PREDICATETRANSFORMER_H_

#include <llvm/Support/Casting.h>

#include "Annotation/Annotation.def"
#include "Factory/Nest.h"
#include "Term/TermBuilder.h"
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

#define HANDLE_ANNOTATION(I, NAME, CLASS) friend class CLASS;
#define HANDLE_ANNOTATION_WITH_BASE(I, BASE, NAME, CLASS) \
friend class BASE; \
friend class CLASS;
#include "Annotation/Annotation.def"

protected:

    FactoryNest FN;

    // FIXME: make this a mixin or smth?
    TermBuilder builder(Term::Ptr term) { return { FN.Term, term }; }
    TermBuilder builder(long long val) { return { FN.Term, FN.Term->getOpaqueConstantTerm(val) }; }
    TermBuilder null() { return { FN.Term, FN.Term->getNullPtrTerm() }; }
    TermBuilder invalid() { return { FN.Term, FN.Term->getInvalidPtrTerm() }; }


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
        CLASS##Ptr pp = std::static_pointer_cast<const CLASS>(p->accept(this)); \
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

    ////////////////////////////////////////////////////////////////////////////
    //
    // Annotations
    //
    ////////////////////////////////////////////////////////////////////////////

public:

    Annotation::Ptr transform(Annotation::Ptr anno) {
        DELEGATE(Base, anno);
    }

protected:

    Annotation::Ptr transformBase(Annotation::Ptr anno) {
        Annotation::Ptr res;
        if(false) {}
#define HANDLE_ANNOTATION(IGNORE, NAME, CLASS) \
        else if (llvm::isa<CLASS>(anno)) { \
            res = static_cast<SubClass*>(this)-> \
                transform##NAME(std::static_pointer_cast<const CLASS>(anno)); \
        }
#define HANDLE_ANNOTATION_WITH_BASE(IGNORE, BASE, NAME, CLASS) /* We handle bases by hand, sorry =( */
#include "Annotation/Annotation.def"
        else if(llvm::isa<LogicAnnotation>(anno)) {
            res = static_cast<SubClass*>(this)->
                transformLogic(std::static_pointer_cast<const LogicAnnotation>(anno));
        }
        ASSERT(res, "Unsupported annotation type");
        DELEGATE(Annotation, res);
    }

    typedef std::shared_ptr<const LogicAnnotation> LogicAnnotationPtr;
    Annotation::Ptr transformLogic(LogicAnnotationPtr anno) {
        Annotation::Ptr res;
#define HANDLE_LogicAnnotation(NAME, CLASS) \
        if (llvm::isa<CLASS>(anno)) { \
            res = static_cast<SubClass*>(this)-> \
                transform##NAME(std::static_pointer_cast<const CLASS>(anno)); \
        }
#define HANDLE_ANNOTATION(A, B, C)
#define HANDLE_ANNOTATION_WITH_BASE(IGNORE, BASE, NAME, CLASS) HANDLE_ ## BASE(NAME, CLASS)
#include "Annotation/Annotation.def"
#undef HANDLE_LogicAnnotation
        return res;
    }

    Annotation::Ptr transformLogicAnnotation(LogicAnnotationPtr anno) {
        return anno;
    }

    Annotation::Ptr transformAnnotation(Annotation::Ptr a) {
        return a;
    }

#define HANDLE_ANNOTATION(I, NAME, CLASS) \
    typedef std::shared_ptr<const CLASS> CLASS##Ptr; \
    Annotation::Ptr transform##NAME(CLASS##Ptr a) { \
        CLASS##Ptr ta = std::static_pointer_cast<const CLASS>(a->accept(this)); \
        DELEGATE(CLASS, ta); \
    }
#include "Annotation/Annotation.def"

#define HANDLE_ANNOTATION(I, NAME, CLASS) \
    Annotation::Ptr transform##CLASS(CLASS##Ptr t) { \
        return t; \
    }
#define HANDLE_ANNOTATION_WITH_BASE(IGNORE, BASE, NAME, CLASS) \
    Annotation::Ptr transform##CLASS(CLASS##Ptr t) { \
        DELEGATE(BASE, t); \
    }
#include "Annotation/Annotation.def"
#undef HANDLE_LogicAnnotation

};

#ifdef DELEGATE
#undef DELEGATE
#endif

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* PREDICATETRANSFORMER_H_ */
