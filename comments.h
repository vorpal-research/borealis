/*
 * comments.h
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#ifndef COMMENTS_H_
#define COMMENTS_H_

#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"

namespace comments{
	class CommentKeeper: public virtual clang::CommentHandler {
		std::list<std::pair<clang::SourceRange, llvm::StringRef>> comments;

	public:
		virtual bool HandleComment(clang::Preprocessor &PP, clang::SourceRange Comment);
		virtual ~CommentKeeper(){};
	};
}

#endif /* COMMENTS_H_ */
