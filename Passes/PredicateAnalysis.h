/*
 * PredicateAnalysis.h
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#ifndef PREDICATEANALYSIS_H_
#define PREDICATEANALYSIS_H_

#include <llvm/Instructions.h>
#include <llvm/Function.h>
#include <llvm/Pass.h>

#include <map>
#include <set>
#include <vector>

#include "../util.hpp"

namespace borealis {

class PredicateAnalysis: public llvm::FunctionPass {

public:

	typedef std::map<const llvm::Instruction*, std::string> PredicateMap;
	typedef std::pair<const llvm::Instruction*, std::string> PredicateMapEntry;

	static char ID;

	PredicateAnalysis();
	virtual bool runOnFunction(llvm::Function& F);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
	virtual ~PredicateAnalysis();

	PredicateMap& getPredicateMap() {
		return predicateMap;
	}

private:

	PredicateMap predicateMap;

	void processInst(const llvm::Instruction& I);
	void process(const llvm::BranchInst& I);
	void process(const llvm::ICmpInst& I);
	void process(const llvm::LoadInst& I);
	void process(const llvm::StoreInst& I);

};

} /* namespace borealis */

#endif /* PREDICATEANALYSIS_H_ */
