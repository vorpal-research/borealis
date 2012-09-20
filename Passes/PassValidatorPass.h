/*
 * PassValidatorPass.h
 *
 *  Created on: Aug 31, 2012
 *      Author: belyaev
 */

#ifndef PASSVALIDATORPASS_H_
#define PASSVALIDATORPASS_H_

#include <llvm/Function.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "../util.hpp"

namespace borealis {

struct PassValidatorPass: public llvm::ModulePass {
	static char ID;

	PassValidatorPass();
	virtual bool runOnModule(llvm::Module& F);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
	virtual ~PassValidatorPass();
};


} /* namespace borealis */


#endif /* PASSVALIDATORPASS_H_ */
