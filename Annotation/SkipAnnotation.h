/*
 * SkipAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef SKIPANNOTATION_H_
#define SKIPANNOTATION_H_

#include "Annotation/Annotation.h"
#include "Annotation/AnnotationNames.hpp"

namespace borealis {

/** protobuf -> Annotation/SkipAnnotation.proto
import "Annotation/Annotation.proto";

package borealis.proto;

message SkipAnnotation {
    extend borealis.proto.Annotation {
        optional SkipAnnotation ext = $COUNTER_ANNOTATION;
    }
}

**/
class SkipAnnotation: public Annotation {
    typedef SkipAnnotation self;

public:
    SkipAnnotation(const Locus& locus):
        Annotation(type_id(*this), AnnotationNames<self>::name(), locus) {};
    virtual ~SkipAnnotation() {}

    static bool classof(const Annotation* a) {
        return a->getTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::string&, const std::vector<Term::Ptr>&) {
        return Annotation::Ptr{ new self(locus) };
    }
};

} /* namespace borealis */

#endif /* SKIPANNOTATION_H_ */
