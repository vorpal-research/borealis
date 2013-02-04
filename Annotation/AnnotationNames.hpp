/*
 * AnnotationNames.h
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATIONNAMES_H_
#define ANNOTATIONNAMES_H_

namespace borealis {

#define HANDLE_ANNOTATION(CMD, CLASS) \
    class CLASS;
#include "Annotation.def"

template<class Annotation>
struct AnnotationNames;

#define HANDLE_ANNOTATION(CMD, CLASS) \
    template<> struct AnnotationNames<CLASS> { \
        static constexpr decltype(CMD) name() { return CMD; } \
    };
#include "Annotation.def"

} /* namespace borealis */

#endif /* ANNOTATIONNAMES_H_ */
