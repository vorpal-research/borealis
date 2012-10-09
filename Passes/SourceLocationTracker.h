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


#include <map>
#include <tuple>

namespace borealis {

class SourceLocationTracker: public llvm::ModulePass {

public:
	typedef std::tuple<std::string, unsigned, unsigned> location_t;
	typedef std::map<const llvm::Value*, location_t> valueDebugMap;
	typedef valueDebugMap::value_type valueDebugMapEntry;
private:
	valueDebugMap valueDebugInfo;

public:
	static char ID;

	SourceLocationTracker(): llvm::ModulePass(ID) {}

	virtual void print(llvm::raw_ostream &O, const llvm::Module *M) const;
	virtual bool runOnModule(llvm::Module& M);
	const std::string& getFilenameFor(const llvm::Value* val) const;
	unsigned getLineFor(const llvm::Value* val) const;
	unsigned getColumnFor(const llvm::Value* val) const;
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;

	virtual ~SourceLocationTracker() {}
};

}


#endif /* SOURCELOCATIONTRACKER_H_ */
