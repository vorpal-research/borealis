/*
 * AnnotatedModule.h
 *
 *  Created on: Sep 4, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATEDMODULE_H_
#define ANNOTATEDMODULE_H_

#include <llvm/IR/Module.h>

#include <memory>

#include "Annotation/AnnotationContainer.h"
#include "Actions/VariableInfoFinder.h"

namespace borealis {
namespace driver {

struct PortableModule {
    std::shared_ptr<llvm::Module> module;
    AnnotationContainer::Ptr annotations;
    ExtVariableInfoData extVars;

    typedef std::shared_ptr<PortableModule> Ptr;
};

} // namespace driver
} // namespace borealis

#endif /* ANNOTATEDMODULE_H_ */
