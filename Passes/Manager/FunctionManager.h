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

namespace borealis {

class FunctionManager :
        public llvm::ModulePass,
        public borealis::logging::ClassLevelLogging<FunctionManager> {

    struct FunctionDesc {
        PredicateState::Ptr Req;
        PredicateState::Ptr Bdy;
        PredicateState::Ptr Ens;

        FunctionDesc() {}

        FunctionDesc(PredicateState::Ptr Req, PredicateState::Ptr Bdy, PredicateState::Ptr Ens) :
            Req(Req), Bdy(Bdy), Ens(Ens) {}

        FunctionDesc(PredicateState::Ptr state) {
            auto reqRest = state->splitByTypes({PredicateType::REQUIRES});
            Req = reqRest.first;

            auto ensRest = reqRest.second->splitByTypes({PredicateType::ENSURES});
            Ens = ensRest.first;
            Bdy = ensRest.second;
        }
    };

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("fm")
#include "Util/unmacros.h"

    typedef std::unordered_map<const llvm::Function*, FunctionDesc> Data;
    typedef Data::value_type DataEntry;

    typedef std::unordered_map<const llvm::Function*, unsigned int> Ids;

    FunctionManager();
    virtual bool runOnModule(llvm::Module&) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~FunctionManager() {};

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

private:

    mutable Data data;
    Ids ids;

    FactoryNest FN;

    FunctionDesc get(const llvm::Function* F) const;
    FunctionDesc get(const llvm::CallInst& CI, FactoryNest FN) const;

    FunctionDesc mergeFunctionDesc(const FunctionDesc& d1, const FunctionDesc& d2) const;

};

} /* namespace borealis */

#endif /* FUNCTIONMANAGER_H_ */
