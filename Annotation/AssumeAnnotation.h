/*
 * AssumeAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ASSUMEANNOTATION_H_
#define ASSUMEANNOTATION_H_

#include "Annotation/AnnotationNames.hpp"
#include "Annotation/LogicAnnotation.h"

namespace borealis {

/** protobuf -> Annotation/AssumeAnnotation.proto
import "Annotation/LogicAnnotation.proto";

package borealis.proto;

message AssumeAnnotation {
    extend borealis.proto.LogicAnnotation {
        optional AssumeAnnotation ext = 4;
    }
}

**/
class AssumeAnnotation: public LogicAnnotation {
    typedef AssumeAnnotation self;

public:
    AssumeAnnotation(const Locus& locus, Term::Ptr term):
        LogicAnnotation(type_id(*this), locus, AnnotationNames<self>::name(), term) {}
    virtual ~AssumeAnnotation() {}

    static bool classof(const Annotation* a) {
        if (auto* la = llvm::dyn_cast_or_null<LogicAnnotation>(a)) {
            return la->getTypeId() == type_id<self>();
        } else return false;
    }

    static bool classof(const self* /* p */) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::vector<Term::Ptr>& terms) {
        return Annotation::Ptr{ new self(locus, terms.front()) };
    }

    Annotation::Ptr clone(Term::Ptr newTerm) const {
        return Annotation::Ptr{ new self(locus, newTerm) };
    }
};

} /* namespace borealis */

#endif /* ASSUMEANNOTATION_H_ */
