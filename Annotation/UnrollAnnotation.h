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

class UnrollAnnotation: public Annotation {
    typedef UnrollAnnotation self;
    size_t level;

public:
    UnrollAnnotation(const Locus& locus, size_t level):
        Annotation(type_id(*this), AnnotationNames<self>::name(), locus),
        level(level) {};
    virtual ~UnrollAnnotation() {}

    static bool classof(const Annotation* a) {
        return a->getTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::vector<Term::Ptr>& terms) {
        if (auto* depth_p = llvm::dyn_cast<OpaqueIntConstantTerm>(terms.front().get())) {
            return Annotation::Ptr(new self(locus, depth_p->getValue()));
        } else return Annotation::Ptr();
    }
};

} /* namespace borealis */

#endif /* UNROLLANNOTATION_H_ */
