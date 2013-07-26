/*
 * AnnotationCast.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATIONCAST_H_
#define ANNOTATIONCAST_H_

#include "Annotation/Annotation.h"
#include "Anno/command.hpp"

namespace borealis {

Annotation::Ptr fromParseResult(
        const Locus& locus,
        const anno::command& cmd,
        TermFactory::Ptr tf);

} /* namespace borealis */

#endif /* ANNOTATIONCAST_H_ */
