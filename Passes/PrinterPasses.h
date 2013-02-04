/*
 * PrinterPasses.h
 *
 *  Created on: Jan 24, 2013
 *      Author: belyaev
 */

#ifndef PRINTERPASSES_H_
#define PRINTERPASSES_H_

#include <llvm/Pass.h>

namespace borealis {

llvm::Pass* createPrinterFor(const llvm::PassInfo*, llvm::Pass*);

}

#endif /* PRINTERPASSES_H_ */
