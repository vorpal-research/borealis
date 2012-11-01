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

using llvm::errs;

using namespace borealis;
using util::streams::endl;

using std::pair;
using std::make_pair;

typedef borealis::anno::calculator::command_type command;
typedef std::multimap<Locus, command>  comment_container;

static comment_container getRawTextSlow(const clang::SourceManager &SourceMgr, clang::SourceRange Range) {
	using std::cerr;
	using std::endl;
	using borealis::Locus;
	using borealis::LocalLocus;

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
   const char *BufferStart = SourceMgr.getBufferData(BeginFileID,
                                                     &Invalid).data();
   if (Invalid)
     return comment_container();

   using borealis::anno::calculator::parse_command;
   using borealis::anno::calculator::command_type;
   using borealis::util::for_each;
   // FIXME: get rid of intermediate StringRef, pass pointers to parse_command
   auto comment = llvm::StringRef(BufferStart + BeginOffset, Length);
   auto locus = Locus(SourceMgr.getPresumedLoc(Range.getBegin()));
   auto commands = parse_command(comment.str());

   auto ret = comment_container();
   for_each(commands, [&](command&  cmd){
	   auto pr = make_pair(std::move(locus), std::move(cmd));
	   ret.insert(pr);
   });

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
