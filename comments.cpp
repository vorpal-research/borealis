/*
 * comments.cpp
 *
 *  Created on: Aug 22, 2012
 */

#include <utility>

#include "comments.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

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

   // The comment can't begin in one file and end in another.
   assert(BeginFileID == EndFileID);

   bool Invalid = false;
   const char *BufferStart = SourceMgr.getBufferData(BeginFileID,
                                                     &Invalid).data();
   if (Invalid)
     return llvm::StringRef();

   return llvm::StringRef(BufferStart + BeginOffset, Length);
}


bool CommentKeeper::HandleComment(clang::Preprocessor &PP, clang::SourceRange Comment){
    clang::SourceManager& sm = PP.getSourceManager();
    auto raw = getRawTextSlow(sm, Comment);
    llvm::errs() << "Handling a comment:\n"
                 << raw << '\n';
    comments.push_back(std::make_pair(Comment, raw));
    return false;
}


}




