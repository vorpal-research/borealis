/*
 * AssignsAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ASSIGNSANNOTATION_H_
#define ASSIGNSANNOTATION_H_

#include "Annotation/AnnotationNames.hpp"
#include "Annotation/LogicAnnotation.h"

namespace borealis {

/** protobuf -> Annotation/AssignsAnnotation.proto
import "Annotation/LogicAnnotation.proto";

package borealis.proto;

message AssignsAnnotation {
    extend borealis.proto.LogicAnnotation {
        optional AssignsAnnotation ext = $COUNTER_LOGIC_ANNOTATION;
    }
}

**/
class AssignsAnnotation: public LogicAnnotation {
    typedef AssignsAnnotation self;

public:
    AssignsAnnotation(const Locus& locus, const std::string& meta, Term::Ptr term):
        LogicAnnotation(type_id(*this), locus, meta, AnnotationNames<self>::name(), term) {}
    virtual ~AssignsAnnotation() {}

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

#endif /* ASSIGNSANNOTATION_H_ */
