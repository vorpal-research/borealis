/*
 * RequiresAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef REQUIRESANNOTATION_H_
#define REQUIRESANNOTATION_H_

#include "Annotation/AnnotationNames.hpp"
#include "Annotation/LogicAnnotation.h"

namespace borealis {

/** protobuf -> Annotation/RequiresAnnotation.proto
import "Annotation/LogicAnnotation.proto";

package borealis.proto;

message RequiresAnnotation {
    extend borealis.proto.LogicAnnotation {
        optional RequiresAnnotation ext = $COUNTER_LOGIC_ANNOTATION;
    }
}

**/
class RequiresAnnotation: public LogicAnnotation {
    typedef RequiresAnnotation self;

public:
    RequiresAnnotation(const Locus& locus, const std::string& meta, Term::Ptr term):
        LogicAnnotation(type_id(*this), locus, meta, AnnotationNames<self>::name(), term) {}
    virtual ~RequiresAnnotation() {}

    static bool classof(const Annotation* a) {
        if (auto* la = llvm::dyn_cast_or_null<LogicAnnotation>(a)) {
            return la->getTypeId() == type_id<self>();
        } else return false;
    }

    static bool classof(const self*) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::string& meta, const std::vector<Term::Ptr>& terms) {
        return Annotation::Ptr{ new self(locus, meta, terms.front()) };
    }

    Annotation::Ptr clone(Term::Ptr newTerm) const {
        return Annotation::Ptr{ new self(locus, meta, newTerm) };
    }
};

} /* namespace borealis */

#endif /* REQUIRESANNOTATION_H_ */
