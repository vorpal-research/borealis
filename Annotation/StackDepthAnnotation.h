/*
 * StackDepthAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef STACKDEPTHANNOTATION_H_
#define STACKDEPTHANNOTATION_H_

#include "Annotation/Annotation.h"
#include "Annotation/AnnotationNames.hpp"

namespace borealis {

/** protobuf -> Annotation/StackDepthAnnotation.proto
import "Annotation/Annotation.proto";

package borealis.proto;

message StackDepthAnnotation {
    extend borealis.proto.Annotation {
        optional StackDepthAnnotation ext = $COUNTER_ANNOTATION;
    }

    optional uint32 depth = 1;
}

**/
class StackDepthAnnotation: public Annotation {
    typedef StackDepthAnnotation self;
    size_t depth;

public:
    StackDepthAnnotation(const Locus& locus, size_t depth):
        Annotation(type_id(*this), AnnotationNames<self>::name(), locus),
        depth(depth) {};
    virtual ~StackDepthAnnotation() {}

    virtual std::string argToString() const {
        return " " + std::to_string(depth);
    }

    size_t getDepth() const { return depth; }

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

#endif /* STACKDEPTHANNOTATION_H_ */
