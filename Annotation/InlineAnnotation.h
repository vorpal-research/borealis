/*
 * InlineAnnotation.h
 *
 *  Created on: Aug 16, 2013
 *      Author: Mikhail Belyaev
 */

#ifndef INLINEANNOTATION_H_
#define INLINEANNOTATION_H_

#include "Annotation/Annotation.h"
#include "Annotation/AnnotationNames.hpp"

namespace borealis {

class InlineAnnotation: public Annotation {
    typedef InlineAnnotation self;

public:
    InlineAnnotation(const Locus& locus):
        Annotation(type_id(*this), AnnotationNames<self>::name(), locus) {};
    virtual ~InlineAnnotation() {}

    static bool classof(const Annotation* a) {
        return a->getTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::vector<Term::Ptr>&) {
        return Annotation::Ptr{ new self(locus) };
    }
};

} /* namespace borealis */

#endif /* INLINEANNOTATION_H_ */
