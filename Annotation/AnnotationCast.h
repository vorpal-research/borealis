/*
 * AnnotationCast.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATIONCAST_H_
#define ANNOTATIONCAST_H_

#include "Annotation/Annotation.h"
#include "Anno/anno.h"
#include "Anno/command.hpp"

namespace borealis {

Annotation::Ptr fromParseResult(
        const Locus& locus,
        const anno::command& cmd,
        TermFactory::Ptr tf);

Annotation::Ptr fromString(const Locus& locus, const std::string& text, TermFactory::Ptr TF);

} /* namespace borealis */

#endif /* ANNOTATIONCAST_H_ */
