/*
 * FunctionManager.h
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#ifndef FUNCTIONMANAGER_H_
#define FUNCTIONMANAGER_H_

#include <llvm/Pass.h>

#include <unordered_map>

#include "Factory/Nest.h"
#include "Logging/logger.hpp"
#include "Passes/Defect/DefectManager/DefectInfo.h"

namespace borealis {

class FunctionManager :
        public llvm::ModulePass,
        public borealis::logging::ClassLevelLogging<FunctionManager> {

    struct FunctionDesc {
        PredicateState::Ptr Req;
        PredicateState::Ptr Bdy;
        PredicateState::Ptr Ens;

        FunctionDesc() = default;

        FunctionDesc(PredicateState::Ptr Req, PredicateState::Ptr Bdy, PredicateState::Ptr Ens) :
            Req(Req), Bdy(Bdy), Ens(Ens) {}

        FunctionDesc(PredicateState::Ptr state) {
            auto&& reqRest = state->splitByTypes({PredicateType::REQUIRES});
            Req = reqRest.first;

            auto&& ensRest = reqRest.second->splitByTypes({PredicateType::ENSURES});
            Ens = ensRest.first;

            Bdy = ensRest.second;
        }
    };

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("fm")
#include "Util/unmacros.h"

    using FunctionData = std::unordered_map<const llvm::Function*, FunctionDesc>;
    using Ids = std::unordered_map<const llvm::Function*, unsigned int>;
    using Bond = std::pair<PredicateState::Ptr, DefectInfo>;
    using FunctionBonds = std::unordered_multimap<const llvm::Function*, Bond>;

private:

    mutable FunctionData data;
    mutable Ids ids;
    mutable FunctionBonds bonds;

    FactoryNest FN;

public:

    FunctionManager();
    virtual bool runOnModule(llvm::Module&) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~FunctionManager() = default;

    void put(const llvm::Function* F, PredicateState::Ptr state);
    void update(const llvm::Function* F, PredicateState::Ptr state);

    PredicateState::Ptr getReq(const llvm::Function* F) const;
    PredicateState::Ptr getBdy(const llvm::Function* F) const;
    PredicateState::Ptr getEns(const llvm::Function* F) const;

    PredicateState::Ptr getReq(const llvm::CallInst& CI, FactoryNest FN) const;
    PredicateState::Ptr getBdy(const llvm::CallInst& CI, FactoryNest FN) const;
    PredicateState::Ptr getEns(const llvm::CallInst& CI, FactoryNest FN) const;

    unsigned int getId(const llvm::Function* F) const;
    unsigned int getMemoryStart(const llvm::Function* F) const;
    unsigned int getMemoryEnd(const llvm::Function* F) const;
    std::pair<unsigned int, unsigned int> getMemoryBounds(const llvm::Function* F) const;

    void addBond(const llvm::Function* F, const Bond& bond);
    auto getBonds(const llvm::Function* F) const -> decltype(util::view(bonds.equal_range(0)));

private:

    FunctionDesc get(const llvm::Function* F) const;
    FunctionDesc get(const llvm::CallInst& CI, FactoryNest FN) const;

    FunctionDesc mergeFunctionDesc(const FunctionDesc& d1, const FunctionDesc& d2) const;

};

} /* namespace borealis */

#endif /* FUNCTIONMANAGER_H_ */
