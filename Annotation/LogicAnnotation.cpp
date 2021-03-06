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
        const std::string& meta,
        keyword_t keyword,
        borealis::Term::Ptr term):
                Annotation(type_id<LogicAnnotation>(), keyword, locus),
                logic_annotation_type_id(logic_annotation_type_id),
                meta(meta),
                term(term) {}

LogicAnnotation::~LogicAnnotation() {}

#include "Util/macros.h"
Annotation::Ptr LogicAnnotation::clone(Term::Ptr) const {
    BYE_BYE(Annotation::Ptr, "Not implemented");
}
#include "Util/unmacros.h"

} /* namespace borealis */
