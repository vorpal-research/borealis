/*
 * VariableInfoTracker.h
 *
 *  Created on: Jan 11, 2013
 *      Author: belyaev
 */

#ifndef METAINFOTRACKERPASS_H_
#define METAINFOTRACKERPASS_H_

#include <unordered_map>

#include <llvm/Pass.h>
#include "Codegen/CType/CTypeFactory.h"

#include "Codegen/VarInfoContainer.h"

#include "Passes/Util/DataProvider.hpp"
#include "Util/key_ptr.hpp"
#include "Util/util.h"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {

// a pass that lets you track mappings between clang decls and llvm values
// (and does a bunch of other shit as well...)
// not really a pass, just some functionality bundled into llvm pass system
class VariableInfoTracker : public llvm::ModulePass {
    typedef DataProvider<clang::FileManager> sm_t;

    VarInfoContainer vars;

    CTypeFactory CTF;
    std::unordered_map<llvm::Type*, CType::Ptr> typeMapping;

    llvm::LLVMContext* ctx;
    const llvm::Module* m;
    DebugInfoFinder dfi;

public:

    struct ValueDescriptor {
        llvm::Value* val;
        borealis::DIType type;
        bool shouldBeDereferenced;

        bool isInvalid() const {
            return val == nullptr;
        }

        friend std::ostream& operator<<(std::ostream& ost, const ValueDescriptor& vd) {
            if(vd.isInvalid()) return ost << "<invalid vd>";
            return ost << llvm::valueSummary(vd.val) << "::" << vd.type.getName();
        }
    };
    using ValueDescriptors = std::vector<ValueDescriptor>;
    llvm::LLVMContext& getLLVMContext() const { return *ctx; }
    const llvm::Module& getModule() const { return *m; }
    const DebugInfoFinder& getDebugInfo() const { return dfi; }

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
        if (fnd == end) return ValueDescriptor{ nullptr, DIType{}, false };

        Value* foundValue = fnd->second;
        const Locus& foundLocus = *fnd->first;

        DIType type;
        bool deref = false;

        // vars :: llvm::Value* -> (llvm::Value*, VarInfo)
        for (auto& pr : view(vars.get(foundValue))) {
            const VarInfo& varInfo = pr.second;
            if (varInfo.originalLocus == foundLocus) {
                type = varInfo.type;
                deref = (varInfo.treatment == VarInfo::Allocated);
                break;
            }
        }

        return ValueDescriptor{ foundValue, type, deref };
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
                        return ValueDescriptor{
                            foundValue,
                            varInfo.type,
                            (varInfo.treatment == VarInfo::Allocated)
                        };
                    }
                }
            }
        }

        return ValueDescriptor{ nullptr, DIType{}, false };
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

    static bool isGlobal(VarInfoContainer::loc_value_iterator::reference pr) {
        return llvm::isa<llvm::GlobalValue>(pr.second);
    }

public:

    static char ID;

    VariableInfoTracker();
    virtual ~VariableInfoTracker();

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnModule(llvm::Module&) override;
    virtual void print(llvm::raw_ostream&, const llvm::Module* M) const override;

    ValueDescriptors locate(llvm::Value* v) const {
        return borealis::util::view(vars.get(v)).map(
            [](const std::pair<llvm::Value*, VarInfo>& e) {
                return ValueDescriptor {
                    e.first,
                    e.second.type,
                    e.second.treatment == VarInfo::Allocated
                };
            }
        ).toVector();
    }

    ValueDescriptor locate(const Locus& loc, DiscoveryPolicy policy) const {
        ValueDescriptor res;

        switch(policy) {
        case DiscoveryPolicy::NextVal:
            res = locate_simple(vars.byLocFwd(loc), vars.byLocEnd(), isNotFunc);
            break;
        case DiscoveryPolicy::PreviousVal:
            res = locate_simple(vars.byLocReverse(loc), vars.byLocReverseEnd(), isNotFunc);
            break;
        case DiscoveryPolicy::NextFunction:
            res = locate_simple(vars.byLocFwd(loc), vars.byLocEnd(), isFunc);
            break;
        case DiscoveryPolicy::PreviousFunction:
            res = locate_simple(vars.byLocReverse(loc), vars.byLocReverseEnd(), isFunc);
            break;
        case DiscoveryPolicy::NextArgument:
            res = locate_simple(vars.byLocFwd(loc), vars.byLocEnd(), isArg);
            break;
        case DiscoveryPolicy::PreviousArgument:
            res = locate_simple(vars.byLocReverse(loc), vars.byLocReverseEnd(), isArg);
            break;
        default:
            BYE_BYE(ValueDescriptor, "Unknown discovery policy requested");
        }

        return res;
    }

    const VarInfoContainer& getVars() const { return vars; }

    ValueDescriptor locate(const std::string& name, const Locus& loc, DiscoveryPolicy policy) const {
        ValueDescriptor res;

        switch(policy) {
        case DiscoveryPolicy::NextVal:
            res = locate_simple(name, vars.byLocFwd(loc), vars.byLocEnd(), isNotFunc);
            break;
        case DiscoveryPolicy::PreviousVal:
            res = locate_simple(name, vars.byLocReverse(loc), vars.byLocReverseEnd(), isNotFunc);
            break;
        case DiscoveryPolicy::NextFunction:
            res = locate_simple(name, vars.byLocFwd(loc), vars.byLocEnd(), isFunc);
            break;
        case DiscoveryPolicy::PreviousFunction:
            res = locate_simple(name, vars.byLocReverse(loc), vars.byLocReverseEnd(), isFunc);
            break;
        case DiscoveryPolicy::NextArgument:
            res = locate_simple(name, vars.byLocFwd(loc), vars.byLocEnd(), isArg);
            break;
        case DiscoveryPolicy::PreviousArgument:
            res = locate_simple(name, vars.byLocReverse(loc), vars.byLocReverseEnd(), isArg);
            break;
        default:
            BYE_BYE(ValueDescriptor, "Unknown discovery policy requested");
        }

        // Fallback to globals if we can't find anything...
        if (res.isInvalid()) res = locate_simple(name, vars.byLocBegin(), vars.byLocEnd(), isGlobal);

        return res;
    }
};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* METAINFOTRACKERPASS_H_ */
