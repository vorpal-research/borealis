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
#include "util.h"

using llvm::errs;

using namespace borealis;
using util::streams::endl;

namespace borealis {
namespace comments {

llvm::StringRef getRawTextSlow(const clang::SourceManager &SourceMgr, clang::SourceRange Range) {
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
     return llvm::StringRef();

   // A comment can't begin in one file and end in another one
   assert(BeginFileID == EndFileID);

   bool Invalid = false;
   const char *BufferStart = SourceMgr.getBufferData(BeginFileID,
                                                     &Invalid).data();
   if (Invalid)
     return llvm::StringRef();

   return llvm::StringRef(BufferStart + BeginOffset, Length);
}

bool GatherCommentsAction::CommentKeeper::HandleComment(clang::Preprocessor &PP, clang::SourceRange Comment) {
    clang::SourceManager& sm = PP.getSourceManager();
    auto raw = getRawTextSlow(sm, Comment);
    comments[Comment] = raw;
    return false;
}

} // namespace comments
} // namespace borealis
