/*
 * LogicAnnotation.cpp
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#include "Annotation/LogicAnnotation.h"

namespace borealis {

LogicAnnotation::LogicAnnotation(
        borealis::id_t logic_annotation_type_id,
        const Locus& locus,
        keyword_t keyword,
        borealis::Term::Ptr term):
                Annotation(type_id<LogicAnnotation>(), keyword, locus),
                logic_annotation_type_id(logic_annotation_type_id),
                term(term) {}

LogicAnnotation::~LogicAnnotation() {}

Annotation::Ptr LogicAnnotation::clone(Term::Ptr) const{
#include "Util/macros.h"
    BYE_BYE(Annotation::Ptr, "Not implemented")
#include "Util/unmacros.h"

}

} /* namespace borealis */
