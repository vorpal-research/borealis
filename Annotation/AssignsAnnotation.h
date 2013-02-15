/*
 * AssignsAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ASSIGNSANNOTATION_H_
#define ASSIGNSANNOTATION_H_

#include "Annotation/AnnotationNames.hpp"
#include "Annotation/LogicAnnotation.h"

namespace borealis {

class AssignsAnnotation: public LogicAnnotation {
    typedef AssignsAnnotation self;

public:
    AssignsAnnotation(const Locus& locus, Term::Ptr term):
        LogicAnnotation(type_id(*this), locus, AnnotationNames<self>::name(), term) {}
    virtual ~AssignsAnnotation(){}

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

#endif /* ASSIGNSANNOTATION_H_ */
