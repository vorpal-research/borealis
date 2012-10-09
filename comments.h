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

namespace borealis {
namespace comments {

using llvm::StringRef;

// comparator needed for map<SourceRange,...>
struct CommentRangeCompare: public std::binary_function<clang::SourceRange, clang::SourceRange, bool> {
	bool operator()(const clang::SourceRange& r1, const clang::SourceRange& r2) const {
		return r1.getBegin() < r2.getBegin();
	}
};



class GatherCommentsAction: public clang::PreprocessOnlyAction {
private:
	typedef std::map<clang::SourceRange, llvm::StringRef, CommentRangeCompare>  comment_container;

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

	std::map<llvm::StringRef, comment_container> allComments;

public:
	comment_container& getCommentsForFile(StringRef file) {
		return allComments[file];
	}

protected:
	virtual bool BeginSourceFileAction(clang::CompilerInstance &CI,
			llvm::StringRef Filename) {
		auto& PP = CI.getPreprocessor();
		currentFile = Filename;
		keeper.reset(new CommentKeeper(PP));
		keeper->attach();
		return true;
	}

	virtual void EndSourceFileAction() {
		allComments[currentFile] = keeper->detach();
	}
};

} // namespace comments
} // namespace borealis

#endif /* COMMENTS_H_ */
