/*
 * NameTracker.h
 *
 *  Created on: Oct 11, 2012
 *      Author: belyaev
 */

#ifndef NAMETRACKER_H_
#define NAMETRACKER_H_

#include <llvm/BasicBlock.h>
#include <llvm/Function.h>
#include <llvm/Instruction.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Value.h>

#include <unordered_map>

namespace borealis {

class NameTracker: public llvm::ModulePass {

public:
	typedef std::unordered_map<std::string, llvm::Value*> nameResolver_t;
	typedef std::unordered_map<llvm::Function*, nameResolver_t> localNameResolvers_t;
private:
	nameResolver_t globalResolver;
	localNameResolvers_t localResolvers;
public:
	static char ID;

	NameTracker(): ModulePass(ID) {};
	virtual bool runOnModule(llvm::Module& M);
	virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
	virtual ~NameTracker() {}

	const nameResolver_t& getGlobalResolver() {
		return globalResolver;
	}

	const nameResolver_t& getLocalResolver(llvm::Function* F) {
		return localResolvers[F];
	}

	llvm::Value* resolve(const std::string& name) {
		if (globalResolver.count(name)) return globalResolver[name];
		else return nullptr;
	}

	llvm::Value* resolve(const std::string& name, llvm::Function* context) {
		if (localResolvers.count(context) && localResolvers[context].count(name)) return localResolvers[context][name];
		else return nullptr;
	}
};

} // namespace borealis

#endif /* NAMETRACKER_H_ */
