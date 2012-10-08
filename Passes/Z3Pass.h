/*
 * Z3Pass.h
 *
 *  Created on: Oct 8, 2012
 *      Author: ice-phoenix
 */

#ifndef Z3PASS_H_
#define Z3PASS_H_

#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>

#include "../util.h"

namespace borealis {

class Z3Pass : public llvm::FunctionPass {

public:

	static char ID;

	Z3Pass();
	virtual bool runOnFunction(llvm::Function& F);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
	virtual ~Z3Pass();

};

} /* namespace borealis */

#endif /* Z3PASS_H_ */
