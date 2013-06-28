/*
 * MetaInfoTrackerPass.h
 *
 *  Created on: Jan 11, 2013
 *      Author: belyaev
 */

#ifndef METAINFOTRACKERPASS_H_
#define METAINFOTRACKERPASS_H_

#include <llvm/Pass.h>

#include "Codegen/VarInfoContainer.h"
#include "Passes/Util/DataProvider.hpp"
#include "Util/key_ptr.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

// a pass that lets you track mappings between clang decls and llvm values
// (and does a bunch of other shit as well...)
// not really a pass, just some functionality bundled into llvm pass system
class MetaInfoTracker : public llvm::ModulePass {
    typedef DataProvider<clang::SourceManager> sm_t;

    VarInfoContainer vars;

public:

    struct ValueDescriptor {
        llvm::Value* val;
        bool shouldBeDereferenced;

        bool isInvalid() {
            return val == nullptr;
        }
    };

private:

    template<class Iter>
    ValueDescriptor locate_simple(
            Iter begin,
            Iter end,
            bool(*f)(typename Iter::reference)) const {

        using namespace llvm;
        using borealis::util::view;

        // fnd :: (Locus, llvm::Value*)
        auto fnd = std::find_if(begin, end, f);
        if (fnd == end) return ValueDescriptor{ nullptr, false };

        Value* foundValue = fnd->second;
        const Locus& foundLocus = *fnd->first;

        bool deref = false;

        // vars :: llvm::Value* -> (llvm::Value*, VarInfo)
        for (auto& pr : view(vars.get(foundValue))) {
            const VarInfo& varInfo = pr.second;
            if (varInfo.originalLocus == foundLocus) {
                deref = (varInfo.treatment == VarInfo::Allocated);
                break;
            }
        }

        return ValueDescriptor{ foundValue, deref };
    }

    template<class Iter>
    ValueDescriptor locate_simple(
            const std::string& name,
            Iter begin,
            Iter end,
            bool(*f)(typename Iter::reference)) const {

        using namespace llvm;
        using borealis::util::view;

        // valsByName :: (begin, end) where varName == `name`
        auto valsByName = vars.byName(name);

        // byLoc :: (Locus, llvm::Value*)
        for (auto& byLoc : view(begin, end)) {
            if (!f(byLoc)) continue;

            // byName :: (std::string, llvm::Value*)
            for (auto& byName : view(valsByName)) {
                if (byLoc.second != byName.second) continue;

                Value* foundValue = byLoc.second;
                const Locus& foundLocus = *byLoc.first;
                const std::string& foundName = *byName.first;

                // vars :: llvm::Value* -> (llvm::Value*, VarInfo)
                for (auto& pr : view(vars.get(foundValue))) {
                    const VarInfo& varInfo = pr.second;
                    if (varInfo.originalLocus == foundLocus &&
                        varInfo.originalName == foundName) {
                        return ValueDescriptor{ foundValue, (varInfo.treatment == VarInfo::Allocated) };
                    }
                }
            }
        }

        return ValueDescriptor{ nullptr, false };
    }

    static bool isFunc(VarInfoContainer::loc_value_iterator::reference pr) {
        return llvm::isa<llvm::Function>(pr.second);
    }

    static bool isNotFunc(VarInfoContainer::loc_value_iterator::reference pr) {
        return !llvm::isa<llvm::Function>(pr.second);
    }

    static bool isArg(VarInfoContainer::loc_value_iterator::reference pr) {
        return llvm::isa<llvm::Argument>(pr.second);
    }

public:

    static char ID;

    MetaInfoTracker();
    virtual ~MetaInfoTracker();

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnModule(llvm::Module&) override;
    virtual void print(llvm::raw_ostream&, const llvm::Module* M) const override;

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
        case DiscoveryPolicy::NextArgument:
            return locate_simple(vars.byLocFwd(loc), vars.byLocEnd(), isArg);
        case DiscoveryPolicy::PreviousArgument:
            return locate_simple(vars.byLocReverse(loc), vars.byLocReverseEnd(), isArg);
        default:
            BYE_BYE(ValueDescriptor, "Unknown discovery policy requested");
        }
    }

    ValueDescriptor locate(const std::string& name, const Locus& loc, DiscoveryPolicy policy) const {
        switch(policy) {
        case DiscoveryPolicy::NextVal:
            return locate_simple(name, vars.byLocFwd(loc), vars.byLocEnd(), isNotFunc);
        case DiscoveryPolicy::PreviousVal:
            return locate_simple(name, vars.byLocReverse(loc), vars.byLocReverseEnd(), isNotFunc);
        case DiscoveryPolicy::NextFunction:
            return locate_simple(name, vars.byLocFwd(loc), vars.byLocEnd(), isFunc);
        case DiscoveryPolicy::PreviousFunction:
            return locate_simple(name, vars.byLocReverse(loc), vars.byLocReverseEnd(), isFunc);
        case DiscoveryPolicy::NextArgument:
            return locate_simple(name, vars.byLocFwd(loc), vars.byLocEnd(), isArg);
        case DiscoveryPolicy::PreviousArgument:
            return locate_simple(name, vars.byLocReverse(loc), vars.byLocReverseEnd(), isArg);
        default:
            BYE_BYE(ValueDescriptor, "Unknown discovery policy requested");
        }
    }
};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* METAINFOTRACKERPASS_H_ */
