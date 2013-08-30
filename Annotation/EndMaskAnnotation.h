/*
 * EndMaskAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ENDMASKANNOTATION_H_
#define ENDMASKANNOTATION_H_

#include "Annotation/Annotation.h"
#include "Annotation/AnnotationNames.hpp"

namespace borealis {

/** protobuf -> Annotation/EndMaskAnnotation.proto
import "Annotation/Annotation.proto";

package borealis.proto;

message EndMaskAnnotation {
    extend borealis.proto.Annotation {
        optional EndMaskAnnotation ext = 2;
    }
}

**/
class EndMaskAnnotation: public Annotation {
    typedef EndMaskAnnotation self;

public:
    EndMaskAnnotation(const Locus& locus):
        Annotation(type_id(*this), AnnotationNames<self>::name(), locus) {};
    virtual ~EndMaskAnnotation() {}

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

#endif /* ENDMASKANNOTATION_H_ */
