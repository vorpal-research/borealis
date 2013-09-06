/*
 * AnnotatedModule.h
 *
 *  Created on: Sep 4, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATEDMODULE_H_
#define ANNOTATEDMODULE_H_

#include "Annotation/AnnotationContainer.h"

namespace borealis {
namespace driver {

struct AnnotatedModule {
    std::shared_ptr<llvm::Module> module;
    AnnotationContainer::Ptr annotations;

    typedef std::shared_ptr<AnnotatedModule> Ptr;
};

}
}

#endif /* ANNOTATEDMODULE_H_ */
