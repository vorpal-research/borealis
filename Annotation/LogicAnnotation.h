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

/** protobuf -> Annotation/LogicAnnotation.proto
import "Annotation/Annotation.proto";
import "Term/Term.proto";

package borealis.proto;

message LogicAnnotation {
    extend borealis.proto.Annotation {
        optional LogicAnnotation ext = 5;
    }

    optional borealis.proto.Term term = 1;
    extensions 2 to 16;
}

**/
class LogicAnnotation: public Annotation {
    typedef LogicAnnotation self;

protected:
    borealis::id_t logic_annotation_type_id;
    Term::Ptr term;

public:
    LogicAnnotation(
            id_t logic_annotation_type_id,
            const Locus& locus,
            keyword_t keyword,
            Term::Ptr term);
    virtual ~LogicAnnotation() = 0;

    Term::Ptr getTerm() const { return term; }
    id_t getTypeId() const { return logic_annotation_type_id; }

    virtual std::string argToString() const {
        return " " + term->getName();
    }

    virtual Annotation::Ptr clone(Term::Ptr newTerm) const = 0;

    template<class SubClass>
    Annotation::Ptr accept(Transformer<SubClass>* t) const {
        auto term_ = t->transform(term);
        if(term != term_) return this->clone(term_);
        else return shared_from_this();
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
