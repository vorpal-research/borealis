/*
 * SourceLocationTracker.h
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#ifndef SOURCELOCATIONTRACKER_H_
#define SOURCELOCATIONTRACKER_H_

#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Analysis/DebugInfo.h>

#include "../util.h"
#include "SourceLocationTracker/location_container.hpp"

#include <map>
#include <tuple>

namespace borealis {

using llvm::Value;

class SourceLocationTracker: public llvm::ModulePass {

public:
	typedef Locus location_t;
	typedef location_container<Value*> valueDebugMap;
private:
	valueDebugMap valueDebugInfo;

public:
	static char ID;

	SourceLocationTracker(): llvm::ModulePass(ID) {}

	virtual void print(llvm::raw_ostream &O, const llvm::Module *M) const;
	virtual bool runOnModule(llvm::Module& M);
	const std::string& getFilenameFor(llvm::Value* val) const;
	unsigned getLineFor(llvm::Value* val) const;
	unsigned getColumnFor(llvm::Value* val) const;
	const Locus& getLocFor(llvm::Value* val) const;

	valueDebugMap::const_range getRangeFor(const Locus& loc) const;

	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;

	virtual ~SourceLocationTracker() {}
};

}


#endif /* SOURCELOCATIONTRACKER_H_ */
