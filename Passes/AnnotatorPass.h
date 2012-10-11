/*
 * Annotator.h
 *
 *  Created on: Oct 8, 2012
 *      Author: belyaev
 */
#ifndef ANNOTATOR_H
#define ANNOTATOR_H

#include <unordered_map>

#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "../Anno/anno.h"
#include "DataProvider.hpp"
#include "SourceLocationTracker.h"
#include "NameTracker.h"
#include "../comments.h"

namespace borealis {

class AnnotatorPass: public llvm::ModulePass {
public:
	static char ID;

	AnnotatorPass(): llvm::ModulePass(ID) {};

	typedef borealis::DataProvider<borealis::comments::GatherCommentsAction> comments;
	typedef borealis::SourceLocationTracker locs;
	typedef borealis::NameTracker names;
	typedef unordered_map<Value*, borealis::anno::command> annotations_t;

private:
	annotations_t annotations;
public:

	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const{
		using borealis::DataProvider;
		using borealis::comments::GatherCommentsAction;

		Info.addRequiredTransitive< comments >();
		Info.addRequiredTransitive< names >();
		Info.addRequiredTransitive< locs >();
	}

	virtual bool runOnModule(llvm::Module&) {
		using borealis::DataProvider;
		using borealis::comments::GatherCommentsAction;

		using llvm::errs;
		using borealis::util::streams::endl;
		using borealis::util::toString;

		auto& commentsPass = getAnalysis< comments >();
		auto& locPass = getAnalysis< locs >();

		for(const auto & Comment: commentsPass.provide().getComments()) {
			const auto & loc = Comment.first;
			const auto & cmd = Comment.second;

			auto values = locPass.getRangeFor(loc);
			for(auto it = values.first; it != values.second; ++it) {
				annotations[it->second] = cmd;
			}
		}


		return false;
	}

	virtual void print(llvm::raw_ostream& O, const llvm::Module*) const {
		using borealis::util::streams::endl;
		using borealis::util::toString;

		for(const auto& An: annotations) {
			O << *An.first << endl;
			O << toString(An.second) << endl;
		}
	}

};

}

#endif // ANNOTATOR_H
