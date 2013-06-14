/*
 * IgnoreAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef IGNOREANNOTATION_H_
#define IGNOREANNOTATION_H_

#include "Annotation/Annotation.h"
#include "Annotation/AnnotationNames.hpp"

namespace borealis {

class IgnoreAnnotation: public Annotation {
    typedef IgnoreAnnotation self;

public:
    IgnoreAnnotation(const Locus& locus):
        Annotation(type_id(*this), AnnotationNames<self>::name(), locus) {};
    virtual ~IgnoreAnnotation() {}

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

#endif /* IGNOREANNOTATION_H_ */
