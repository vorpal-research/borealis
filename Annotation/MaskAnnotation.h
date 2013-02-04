/*
 * MaskAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef MASKANNOTATION_H_
#define MASKANNOTATION_H_

#include "Annotation/Annotation.h"
#include "Annotation/AnnotationNames.hpp"

namespace borealis {

class MaskAnnotation: public Annotation {
    typedef MaskAnnotation self;

    std::vector<Term::Ptr> masks;

public:
    MaskAnnotation(const Locus& locus, const std::vector<Term::Ptr>& masks):
        Annotation(type_id(*this), AnnotationNames<self>::name(), locus),
        masks(masks) {};
    virtual ~MaskAnnotation() {}

    static bool classof(const Annotation* a) {
        return a->getTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::vector<Term::Ptr>& terms) {
        return Annotation::Ptr(new self(locus, terms));
    }
};

} /* namespace borealis */

#endif /* MASKANNOTATION_H_ */
