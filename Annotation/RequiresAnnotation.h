/*
 * RequiresAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef REQUIRESANNOTATION_H_
#define REQUIRESANNOTATION_H_

#include "LogicAnnotation.h"
#include "AnnotationNames.hpp"

namespace borealis {

class RequiresAnnotation: public LogicAnnotation {
    typedef RequiresAnnotation self;
public:
    RequiresAnnotation(const Locus& locus, Term::Ptr term):
        LogicAnnotation(type_id(*this), locus, AnnotationNames<self>::name(), term) {}
    virtual ~RequiresAnnotation(){}

    static bool classof(const Annotation* a) {
        if(auto* la = llvm::dyn_cast_or_null<LogicAnnotation>(a)) {
            return la->getTypeId() == type_id<self>();
        } else return false;
    }

    static bool classof(const self* /* p */) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::vector<Term::Ptr>& terms) {
        return Annotation::Ptr(new self(locus, terms.front()));
    }
};

} /* namespace borealis */
#endif /* REQUIRESANNOTATION_H_ */
