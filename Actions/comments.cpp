/*
 * comments.cpp
 *
 *  Created on: Aug 22, 2012
 */

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/OwningPtr.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>

#include <utility>

#include "comments.h"
#include "Util/util.h"

namespace borealis {
namespace comments {

using borealis::errs;
using borealis::endl;
using borealis::Locus;
using borealis::LocalLocus;

typedef GatherCommentsAction::comment_container comment_container;

static comment_container getRawTextSlow(const clang::SourceManager &SourceMgr, clang::SourceRange Range) {
	if(SourceMgr.isInSystemHeader(Range.getBegin())) {
		return comment_container();
	}

   clang::FileID BeginFileID;
   clang::FileID EndFileID;
   unsigned BeginOffset;
   unsigned EndOffset;

   llvm::tie(BeginFileID, BeginOffset) =
       SourceMgr.getDecomposedLoc(Range.getBegin());
   llvm::tie(EndFileID, EndOffset) =
       SourceMgr.getDecomposedLoc(Range.getEnd());

   const unsigned Length = EndOffset - BeginOffset;
   if (Length < 2)
     return comment_container();

   // A comment can't begin in one file and end in another one
   assert(BeginFileID == EndFileID);

   bool Invalid = false;
   const char *BufferStart =
           SourceMgr.getBufferData(BeginFileID, &Invalid).data();
   if (Invalid)
     return comment_container();

   using borealis::anno::parse;

   auto comment = llvm::StringRef(BufferStart + BeginOffset, Length);
   auto locus = Locus(SourceMgr.getPresumedLoc(Range.getBegin()));
   auto commands = parse(comment.str());

   auto ret = comment_container();
   for (command& cmd : commands) {
	   auto pr = std::make_pair(locus, std::move(cmd));
	   ret.insert(pr);
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
