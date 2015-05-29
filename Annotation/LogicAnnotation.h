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
        optional LogicAnnotation ext = $COUNTER_ANNOTATION;
    }

    optional borealis.proto.Term term = 1;
    optional string meta = 2;

    extensions 16 to 64;
}

**/
class LogicAnnotation: public Annotation {
    typedef LogicAnnotation self;

protected:
    borealis::id_t logic_annotation_type_id;
    std::string meta;
    Term::Ptr term;

public:
    LogicAnnotation(
            id_t logic_annotation_type_id,
            const Locus& locus,
            const std::string& meta,
            keyword_t keyword,
            Term::Ptr term);
    virtual ~LogicAnnotation() = 0;

    Term::Ptr getTerm() const { return term; }
    const std::string& getMeta() const { return meta; }
    id_t getTypeId() const { return logic_annotation_type_id; }

    virtual std::string argToString() const {
        std::string metaStr = (meta.empty())? "" : "[[" + meta + "]]";
        return std::move(metaStr) + " " + term->getName();
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
