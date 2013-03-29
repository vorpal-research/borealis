/*
 * SourceLocationTracker.h
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#ifndef SOURCELOCATIONTRACKER_H_
#define SOURCELOCATIONTRACKER_H_

#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <tuple>

#include "SourceLocationTracker/location_container.hpp"
#include "Util/util.h"

namespace borealis {

class SourceLocationTracker: public llvm::ModulePass {

public:

	typedef Locus location_t;
	typedef location_container<llvm::Value*> valueDebugMap;
	typedef location_container< std::vector<llvm::BasicBlock*> > loopDebugMap;

private:

	valueDebugMap valueDebugInfo;
	loopDebugMap loopDebugInfo;

public:

	static char ID;

	SourceLocationTracker() : llvm::ModulePass(ID) {}
	virtual bool runOnModule(llvm::Module& M);
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual ~SourceLocationTracker() {}

    virtual void print(llvm::raw_ostream& O, const llvm::Module* M) const;

	const std::string& getFilenameFor(llvm::Value* val) const;
	unsigned getLineFor(llvm::Value* val) const;
	unsigned getColumnFor(llvm::Value* val) const;
	const Locus& getLocFor(llvm::Value* val) const;
	const Locus& getLocFor(llvm::Loop* loop) const;

	valueDebugMap::const_range getRangeFor(const Locus& loc) const;
	const std::vector<llvm::BasicBlock*>& getLoopFor(const Locus& loc) const;
};

} // namespace borealis

#endif /* SOURCELOCATIONTRACKER_H_ */
