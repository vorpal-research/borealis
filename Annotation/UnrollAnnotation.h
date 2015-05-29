/*
 * UnrollAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef UNROLLANNOTATION_H_
#define UNROLLANNOTATION_H_

#include "Annotation/Annotation.h"
#include "Annotation/AnnotationNames.hpp"

namespace borealis {

/** protobuf -> Annotation/UnrollAnnotation.proto
import "Annotation/Annotation.proto";

package borealis.proto;

message UnrollAnnotation {
    extend borealis.proto.Annotation {
        optional UnrollAnnotation ext = $COUNTER_ANNOTATION;
    }

    optional uint32 level = 1;
}

**/
class UnrollAnnotation: public Annotation {
    typedef UnrollAnnotation self;
    size_t level;

public:
    UnrollAnnotation(const Locus& locus, size_t level):
        Annotation(type_id(*this), AnnotationNames<self>::name(), locus),
        level(level) {};
    virtual ~UnrollAnnotation() {}

    virtual std::string argToString() const {
        return " " + std::to_string(level);
    }

    size_t getLevel() const {
        return level;
    }

    static bool classof(const Annotation* a) {
        return a->getTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::string&, const std::vector<Term::Ptr>& terms) {
        if (auto* depth_p = llvm::dyn_cast<OpaqueIntConstantTerm>(terms.front().get())) {
            return Annotation::Ptr{ new self(locus, depth_p->getValue()) };
        } else return Annotation::Ptr{};
    }
};

} /* namespace borealis */

#endif /* UNROLLANNOTATION_H_ */
