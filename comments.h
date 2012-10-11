/*
 * comments.h
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#ifndef COMMENTS_H_
#define COMMENTS_H_

#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/Preprocessor.h>

#include <map>

#include "locations.h"
#include "Anno/anno.h"

namespace borealis {
namespace comments {

using llvm::StringRef;
using borealis::Locus;
typedef borealis::anno::calculator::command_type command;

class GatherCommentsAction: public clang::PreprocessOnlyAction {
private:

	// std::map is here not because it's associative, but 'cos it is sorted and easy-to-use
	// std::set<pair> may seem more applicable, but requires more boilerplate
	typedef std::multimap<Locus, command>  comment_container;
	typedef comment_container::iterator iterator;
	typedef std::pair<iterator,iterator> range;

	class CommentKeeper: public virtual clang::CommentHandler {
		clang::Preprocessor& pp;
		comment_container comments;
		bool attached;

	public:
		CommentKeeper(clang::Preprocessor& ppi): pp(ppi), attached(false) {};

		virtual bool HandleComment(clang::Preprocessor &PP, clang::SourceRange Comment);
		virtual ~CommentKeeper(){ detach(); };

		void attach() {
			if(!attached) pp.AddCommentHandler(this);
			attached = true;
		}

		comment_container detach() {
			if(attached) pp.RemoveCommentHandler(this);
			attached = false;
			return std::move(comments);
		}
	};

	std::auto_ptr<CommentKeeper> keeper;
	llvm::StringRef currentFile = "";

	comment_container allComments;

public:
	const comment_container& getComments() const {
		return allComments;
	}

protected:
	virtual bool BeginSourceFileAction(clang::CompilerInstance &CI,
			llvm::StringRef Filename) {
		llvm::errs() << Filename << "\n";

		auto& PP = CI.getPreprocessor();
		currentFile = Filename;
		keeper.reset(new CommentKeeper(PP));
		keeper->attach();
		return true;
	}

	virtual void EndSourceFileAction() {
		auto fl = keeper->detach();
		allComments.insert(fl.begin(), fl.end());
	}
};

} // namespace comments
} // namespace borealis

#endif /* COMMENTS_H_ */
