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

/** protobuf -> Annotation/MaskAnnotation.proto
import "Annotation/Annotation.proto";
import "Term/Term.proto";

package borealis.proto;

message MaskAnnotation {
    extend borealis.proto.Annotation {
        optional MaskAnnotation ext = 6;
    }

    repeated borealis.proto.Term masks = 1;
}

**/
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

    const std::vector<Term::Ptr>& getMasks() const { return masks; }

    virtual std::string argToString() const {
        using borealis::util::head;
        using borealis::util::tail;

        std::ostringstream str;
        str << " " << head(masks)->getName();
        for (auto& mask : tail(masks)) {
            str << ", " << mask->getName();
        }
        return str.str();
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::vector<Term::Ptr>& terms) {
        return Annotation::Ptr{ new self(locus, terms) };
    }
};

} /* namespace borealis */

#endif /* MASKANNOTATION_H_ */
