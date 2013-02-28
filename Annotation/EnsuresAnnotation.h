/*
 * EnsuresAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ENSURESANNOTATION_H_
#define ENSURESANNOTATION_H_

#include "Annotation/AnnotationNames.hpp"
#include "Annotation/LogicAnnotation.h"

namespace borealis {

class EnsuresAnnotation: public LogicAnnotation {
    typedef EnsuresAnnotation self;

public:
    EnsuresAnnotation(const Locus& locus, Term::Ptr term):
        LogicAnnotation(type_id(*this), locus, AnnotationNames<self>::name(), term) {}
    virtual ~EnsuresAnnotation(){}

    static bool classof(const Annotation* a) {
        if (auto* la = llvm::dyn_cast_or_null<LogicAnnotation>(a)) {
            return la->getTypeId() == type_id<self>();
        } else return false;
    }

    static bool classof(const self*) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::vector<Term::Ptr>& terms) {
        return Annotation::Ptr{ new self(locus, terms.front()) };
    }

    Annotation::Ptr clone(Term::Ptr newTerm) const {
        return Annotation::Ptr{ new self(locus, newTerm) };
    }
};

} /* namespace borealis */

#endif /* ENSURESANNOTATION_H_ */
