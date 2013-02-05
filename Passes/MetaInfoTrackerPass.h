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

    llvm::Value* locate(const std::string& name, const Locus& loc, DiscoveryPolicy policy) const  {
        using borealis::util::view;
        typedef typename VarInfoContainer::loc_value_iterator::value_type loc_and_val;

        switch(policy) {

        case DiscoveryPolicy::NextVal:
        {
            auto fwd_it = vars.byLocFwd(loc);
            auto end = vars.byLocEnd();
            auto valsByName = vars.byName(name);

            for(auto& byloc: view(fwd_it, end)) {
                if(llvm::isa<llvm::Function>(byloc.second)) continue;

                for(auto& byname: view(valsByName)) {
                    if(byloc.second == byname.second) return byloc.second;
                }
            }

            return nullptr;
        }

        case DiscoveryPolicy::PreviousVal:
        {
            auto bwd_it = vars.byLocReverse(loc);
            auto end = vars.byLocReverseEnd();
            auto valsByName = vars.byName(name);

            for(auto& byloc: view(bwd_it, end)) {
                if(llvm::isa<llvm::Function>(byloc.second)) continue;

                for(auto& byname: view(valsByName)) {
                    if(byloc.second == byname.second) return byloc.second;
                }
            }

            return nullptr;
        }

        case DiscoveryPolicy::NextFunction:
        {
            auto fwd_it = vars.byLocFwd(loc);
            auto end = vars.byLocEnd();
            auto valsByName = vars.byName(name);

            for(auto& byloc: view(fwd_it, end)) {
                if(!llvm::isa<llvm::Function>(byloc.second)) continue;

                for(auto& byname: view(valsByName)) {
                    if(byloc.second == byname.second) return byloc.second;
                }
            }

            return nullptr;
        }

#include "Util/macros.h"
        default:
            BYE_BYE(llvm::Value*, "Unknown discovery policy requested");
            return nullptr;
        }
#include "Util/unmacros.h"

    }

};

} /* namespace borealis */

#endif /* CLANGDECLTRACKERPASS_H_ */
