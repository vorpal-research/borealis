/*
 * ClangDeclTrackerPass.h
 *
 *  Created on: Jan 11, 2013
 *      Author: belyaev
 */

#ifndef CLANGDECLTRACKERPASS_H_
#define CLANGDECLTRACKERPASS_H_

#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Pass.h>

#include "Codegen/VarInfoContainer.h"
#include "Passes/DataProvider.hpp"
#include "Util/key_ptr.hpp"
#include "Util/option.hpp"

namespace borealis {

// a pass that lets you track corresponding clang decls for llvm values
// (and also does a bunch of other shit...)
// not really a pass, just some functionality bundled into pass system
class MetaInfoTrackerPass: public llvm::ModulePass {
    typedef DataProvider<clang::SourceManager> sm_t;
    typedef llvm::LoopInfo loops;

    VarInfoContainer vars;

public:
    static char ID;

    MetaInfoTrackerPass();
    virtual ~MetaInfoTrackerPass();

    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual bool runOnModule(llvm::Module&);

    virtual void print(llvm::raw_ostream&, const llvm::Module* M) const;

};

} /* namespace borealis */

#endif /* CLANGDECLTRACKERPASS_H_ */
