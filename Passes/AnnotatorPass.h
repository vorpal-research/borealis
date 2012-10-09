/*
 * Annotator.h
 *
 *  Created on: Oct 8, 2012
 *      Author: belyaev
 */
#ifndef ANNOTATOR_H
#define ANNOTATOR_H

#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "../Anno/anno.h"
#include "DataProvider.hpp"
#include "SourceLocationTracker.h"
#include "../comments.h"

namespace borealis {

class AnnotatorPass: public llvm::ModulePass {
public:
	static char ID;

	AnnotatorPass(): llvm::ModulePass(ID) {};

	typedef borealis::DataProvider<borealis::comments::GatherCommentsAction> comments;
	typedef borealis::SourceLocationTracker locs;

	virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const{
		using borealis::DataProvider;
		using borealis::comments::GatherCommentsAction;

		Info.addRequiredTransitive< comments >();
		Info.addRequiredTransitive< locs >();
	}

	virtual bool runOnModule(llvm::Module&) {
		using borealis::DataProvider;
		using borealis::comments::GatherCommentsAction;

		auto& commentsPass = getAnalysis< comments >();
		auto& locPass = getAnalysis< locs >();

		locPass.print(llvm::errs(), nullptr);


		using borealis::anno::calculator::parse_command;

		auto parsed = parse_command("/* @ensures (2+x*3) \n" \
				" @stack-depth   25   */");

		if(!parsed.empty()) {
			for(auto i = 0U; i < parsed.size(); ++i) {
				std::cerr << parsed[i] << std::endl;
			}

		} else {
			std::cerr << "Fucked up, sorry :(" << std::endl;
		}

		return false;
	}

};

}

#endif // ANNOTATOR_H
