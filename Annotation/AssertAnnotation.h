/*
 * AssertAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ASSERTANNOTATION_H_
#define ASSERTANNOTATION_H_

#include "Annotation/AnnotationNames.hpp"
#include "Annotation/LogicAnnotation.h"

namespace borealis {

class AssertAnnotation: public LogicAnnotation {
    typedef AssertAnnotation self;

public:
    AssertAnnotation(const Locus& locus, Term::Ptr term):
        LogicAnnotation(type_id(*this), locus, AnnotationNames<self>::name(), term) {}
    virtual ~AssertAnnotation(){}

    static bool classof(const Annotation* a) {
        if (auto* la = llvm::dyn_cast_or_null<LogicAnnotation>(a)) {
            return la->getTypeId() == type_id<self>();
        } else return false;
    }

    static bool classof(const self* /* p */) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::vector<Term::Ptr>& terms) {
        return Annotation::Ptr(new self(locus, terms.front()));
    }

    Annotation::Ptr clone(Term::Ptr newTerm) const {
        return Annotation::Ptr{ new self(locus, newTerm) };
    }
};

} /* namespace borealis */

#endif /* ASSERTANNOTATION_H_ */
