/*
 * SourceLocationTracker.h
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */

#ifndef SOURCELOCATIONTRACKER_H_
#define SOURCELOCATIONTRACKER_H_

#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "SourceLocationTracker/location_container.hpp"
#include "Util/util.h"

#include <map>
#include <tuple>

namespace borealis {

class SourceLocationTracker: public llvm::ModulePass {

public:

	typedef Locus location_t;
	typedef location_container<llvm::Value*> valueDebugMap;

private:

	valueDebugMap valueDebugInfo;

public:

	static char ID;

	SourceLocationTracker() : llvm::ModulePass(ID) {}
	virtual bool runOnModule(llvm::Module& M);
    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual ~SourceLocationTracker() {}

    virtual void print(llvm::raw_ostream &O, const llvm::Module *M) const;

	const std::string& getFilenameFor(llvm::Value* val) const;
	unsigned getLineFor(llvm::Value* val) const;
	unsigned getColumnFor(llvm::Value* val) const;
	const Locus& getLocFor(llvm::Value* val) const;

	valueDebugMap::const_range getRangeFor(const Locus& loc) const;
};

} // namespace borealis

#endif /* SOURCELOCATIONTRACKER_H_ */
