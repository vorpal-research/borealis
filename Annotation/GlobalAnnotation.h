/*
 * GlobalAnnotation.h
 *
 *  Created on: Oct 15, 2015
 *      Author: belyaev
 */

#ifndef GLOBALANNOTATION_H_
#define GLOBALANNOTATION_H_

#include "Annotation/AnnotationNames.hpp"
#include "Annotation/LogicAnnotation.h"

namespace borealis {

/** protobuf -> Annotation/GlobalAnnotation.proto
import "Annotation/LogicAnnotation.proto";

package borealis.proto;

message GlobalAnnotation {
    extend borealis.proto.LogicAnnotation {
        optional GlobalAnnotation ext = $COUNTER_LOGIC_ANNOTATION;
    }
}

**/
class GlobalAnnotation: public LogicAnnotation {
    typedef GlobalAnnotation self;

public:
    GlobalAnnotation(const Locus& locus, const std::string& meta, Term::Ptr term):
        LogicAnnotation(type_id(*this), locus, meta, AnnotationNames<self>::name(), term) {}
    virtual ~GlobalAnnotation() {}

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

#endif /* GLOBALANNOTATION_H_ */
