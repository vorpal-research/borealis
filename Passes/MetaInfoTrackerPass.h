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

    struct ValueDescriptor {
        llvm::Value* val;
        bool shouldBeDereferenced;
    };

private:

    template<class Iter>
    ValueDescriptor locate_simple(Iter begin,
            Iter end,
            bool(*f)(typename Iter::reference)) const {
        auto fnd = std::find_if(begin, end, f);

        if(fnd == end) return ValueDescriptor{nullptr, false};

        bool deref = false;

        for(auto& pr : borealis::util::view(vars.get(fnd->second))) {
            if(pr.second.originalLocus == *fnd->first) {
                deref = (pr.second.treatment == VarInfo::Allocated);
                break;
            }
        }

        return ValueDescriptor{fnd->second, deref};
    }

    template<class Iter>
    ValueDescriptor locate_simple(
            const std::string& name,
            Iter begin,
            Iter end,
            bool(*f)(typename Iter::reference)) const {
        auto valsByName = vars.byName(name);

        for(auto& byloc: borealis::util::view(begin, end)) {
            if(!f(byloc)) continue;

            for(auto& byname: view(valsByName)) {
                if(byloc.second == byname.second) {
                    for(auto& pr : borealis::util::view(vars.get(byloc.second))) {
                        if(pr.second.originalLocus == *byloc.first &&
                                pr.second.originalName == *byname.first
                        ) {
                            return ValueDescriptor{byloc.second, (pr.second.treatment == VarInfo::Allocated)};
                        }
                    }
                }
            }
        }

        return ValueDescriptor{nullptr, false};
    }


    static bool isFunc(VarInfoContainer::loc_value_iterator::reference pr) {
        return llvm::isa<llvm::Function>(pr.second);
    }

    static bool isNotFunc(VarInfoContainer::loc_value_iterator::reference pr) {
        return !llvm::isa<llvm::Function>(pr.second);
    }

public:
    static char ID;

    MetaInfoTrackerPass();
    virtual ~MetaInfoTrackerPass();

    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual bool runOnModule(llvm::Module&);

    virtual void print(llvm::raw_ostream&, const llvm::Module* M) const;

    ValueDescriptor locate(const Locus& loc, DiscoveryPolicy policy) const {
        switch(policy) {
        case DiscoveryPolicy::NextVal:
            return locate_simple(vars.byLocFwd(loc), vars.byLocEnd(), isNotFunc);
        case DiscoveryPolicy::PreviousVal:
            return locate_simple(vars.byLocReverse(loc), vars.byLocReverseEnd(), isNotFunc);
        case DiscoveryPolicy::NextFunction:
            return locate_simple(vars.byLocFwd(loc), vars.byLocEnd(), isFunc);
        case DiscoveryPolicy::PreviousFunction:
            return locate_simple(vars.byLocReverse(loc), vars.byLocReverseEnd(), isFunc);
#include "Util/macros.h"
        default:
        {
            BYE_BYE(ValueDescriptor, "Unknown discovery policy requested");
        }
#include "Util/unmacros.h"
        }// switch(policy)
    }

    ValueDescriptor locate(const std::string& name, const Locus& loc, DiscoveryPolicy policy) const {
        using borealis::util::view;
        typedef typename VarInfoContainer::loc_value_iterator::value_type loc_and_val;

        switch(policy) {

        case DiscoveryPolicy::NextVal:
            return locate_simple(name, vars.byLocFwd(loc), vars.byLocEnd(), isNotFunc);
        case DiscoveryPolicy::PreviousVal:
            return locate_simple(name, vars.byLocReverse(loc), vars.byLocReverseEnd(), isNotFunc);
        case DiscoveryPolicy::NextFunction:
            return locate_simple(name, vars.byLocFwd(loc), vars.byLocEnd(), isFunc);
        case DiscoveryPolicy::PreviousFunction:
            return locate_simple(name, vars.byLocReverse(loc), vars.byLocReverseEnd(), isFunc);

#include "Util/macros.h"
        default:
            BYE_BYE(ValueDescriptor, "Unknown discovery policy requested");
        }
#include "Util/unmacros.h"

    }


};

} /* namespace borealis */

#endif /* CLANGDECLTRACKERPASS_H_ */
