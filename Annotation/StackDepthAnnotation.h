/*
 * StackDepthAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef STACKDEPTHANNOTATION_H_
#define STACKDEPTHANNOTATION_H_

#include "Annotation.h"
#include "AnnotationNames.hpp"

namespace borealis {

class StackDepthAnnotation: public Annotation {
    typedef StackDepthAnnotation self;
    size_t depth;

public:
    StackDepthAnnotation(const Locus& locus, size_t depth):
        Annotation(type_id(*this), AnnotationNames<self>::name(), locus),
        depth(depth) {};
    virtual ~StackDepthAnnotation() {}

    static bool classof(const Annotation* a) {
        return a->getTypeId() == type_id<self>();
    }

    static bool classof(const self* /* p */) {
        return true;
    }

    static Annotation::Ptr fromTerms(const Locus& locus, const std::vector<Term::Ptr>& terms) {
        if(auto* depth_p = llvm::dyn_cast<OpaqueIntConstantTerm>(terms.front().get())) {
            return Annotation::Ptr(new self(locus, depth_p->getValue()));
        } else return Annotation::Ptr();
    }
};

} /* namespace borealis */
#endif /* STACKDEPTHANNOTATION_H_ */
