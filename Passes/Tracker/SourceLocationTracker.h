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
#include <llvm/Pass.h>

#include "SourceLocationTracker/location_container.hpp"

namespace borealis {

class SourceLocationTracker : public llvm::ModulePass {

public:

    typedef Locus location_t;
    typedef location_container< llvm::Value* > valueDebugMap;
    typedef location_container< std::vector<llvm::BasicBlock*> > loopDebugMap;

private:

    valueDebugMap valueDebugInfo;
    loopDebugMap loopDebugInfo;

public:

    static char ID;

    SourceLocationTracker() : llvm::ModulePass(ID) {}
    virtual bool runOnModule(llvm::Module& M) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~SourceLocationTracker() {}

    virtual void print(llvm::raw_ostream& O, const llvm::Module* M) const override;

    const std::string& getFilenameFor(const llvm::Value* val) const;
    unsigned getLineFor(const llvm::Value* val) const;
    unsigned getColumnFor(const llvm::Value* val) const;
    const Locus& getLocFor(const llvm::Value* val) const;
    const Locus& getLocFor(const llvm::Loop* loop) const;

    valueDebugMap::const_range getRangeFor(const Locus& loc) const;
    const std::vector<llvm::BasicBlock*>& getLoopFor(const Locus& loc) const;
};

} // namespace borealis

#endif /* SOURCELOCATIONTRACKER_H_ */
