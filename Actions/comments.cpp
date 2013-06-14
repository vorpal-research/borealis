/*
 * comments.cpp
 *
 *  Created on: Aug 22, 2012
 */

#include <utility>

#include "Actions/comments.h"

#include "Util/macros.h"

namespace borealis {
namespace comments {

using borealis::LocalLocus;
using borealis::Locus;

typedef GatherCommentsAction::comment_container comment_container;

static comment_container getRawTextSlow(const clang::SourceManager &SourceMgr, clang::SourceRange Range) {
	if (SourceMgr.isInSystemHeader(Range.getBegin())) {
		return comment_container();
	}

   clang::FileID BeginFileID;
   clang::FileID EndFileID;
   unsigned BeginOffset;
   unsigned EndOffset;

   std::tie(BeginFileID, BeginOffset) =
       SourceMgr.getDecomposedLoc(Range.getBegin());
   std::tie(EndFileID, EndOffset) =
       SourceMgr.getDecomposedLoc(Range.getEnd());

   const unsigned Length = EndOffset - BeginOffset;
   if (Length < 2) return comment_container();

   ASSERT(BeginFileID == EndFileID,
          "Comment can't begin in one file and end in another one");

   bool Invalid = false;
   const char* BufferStart =
       SourceMgr.getBufferData(BeginFileID, &Invalid).data();
   if (Invalid) return comment_container();

   auto comment = llvm::StringRef(BufferStart + BeginOffset, Length);
   auto locus = Locus(SourceMgr.getPresumedLoc(Range.getBegin()));
   auto commands = borealis::anno::parse(comment.str());

   auto ret = comment_container();
   for (auto& cmd : commands) {
	   ret.insert({locus, std::move(cmd)});
   }

   return ret;
}

bool GatherCommentsAction::CommentKeeper::HandleComment(clang::Preprocessor &PP, clang::SourceRange Comment) {
    clang::SourceManager& sm = PP.getSourceManager();
    auto raw = getRawTextSlow(sm, Comment);
    comments.insert(raw.begin(), raw.end());
    return false;
}

} // namespace comments
} // namespace borealis

#include "Util/unmacros.h"
