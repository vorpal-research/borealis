/*
 * ClangDeclTrackerPass.h
 *
 *  Created on: Jan 11, 2013
 *      Author: belyaev
 */

#ifndef CLANGDECLTRACKERPASS_H_
#define CLANGDECLTRACKERPASS_H_

#include <llvm/Pass.h>
#include <clang/AST/DeclBase.h>
#include <clang/Basic/SourceManager.h>


#include "Passes/DataProvider.hpp"
#include "Codegen/VarInfoContainer.h"
#include "Util/option.hpp"
#include "Util/key_ptr.hpp"

namespace borealis {

// a pass that lets you track corresponding clang decls for llvm values
// not really a pass, just some functionality bundled into pass system
class MetaInfoTrackerPass: public llvm::ModulePass {
    typedef DataProvider<clang::SourceManager> sm_t;
    typedef llvm::LoopInfo loops;

    VarInfoContainer globals;
    std::unordered_map<llvm::Function*, VarInfoContainer > locals;

public:
    static char ID;

    MetaInfoTrackerPass();
    virtual ~MetaInfoTrackerPass();

    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual bool runOnModule(llvm::Module&);

};

} /* namespace borealis */
#endif /* CLANGDECLTRACKERPASS_H_ */
