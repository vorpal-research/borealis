/*
 * LogicAnnotation.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef LOGICANNOTATION_H_
#define LOGICANNOTATION_H_

#include "Annotation/Annotation.h"

namespace borealis {

class LogicAnnotation: public Annotation {
    typedef LogicAnnotation self;

protected:
    borealis::id_t logic_annotation_type_id;
    borealis::Term::Ptr term;

public:
    LogicAnnotation(
            id_t logic_annotation_type_id,
            const Locus& locus,
            keyword_t keyword,
            borealis::Term::Ptr term);
    virtual ~LogicAnnotation() = 0;

    Term::Ptr getTerm() { return term; }
    id_t getTypeId() const { return logic_annotation_type_id; }

    virtual std::string argToString() const {
        return " " + term->getName();
    }

    static bool classof(const Annotation* a) {
        return a->getTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }
};

} /* namespace borealis */

#endif /* LOGICANNOTATION_H_ */
