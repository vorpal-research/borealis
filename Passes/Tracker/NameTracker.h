/*
 * NameTracker.h
 *
 *  Created on: Oct 11, 2012
 *      Author: belyaev
 */

#ifndef NAMETRACKER_H_
#define NAMETRACKER_H_

#include <llvm/Pass.h>
#include <llvm/IR/Value.h>

#include <unordered_map>

#include "Logging/logger.hpp"

namespace borealis {

class NameTracker :
        public llvm::ModulePass,
        public borealis::logging::ClassLevelLogging<NameTracker> {

public:

    typedef std::unordered_map<std::string, llvm::Value*> nameResolver_t;
    typedef std::unordered_map<llvm::Function*, nameResolver_t> localNameResolvers_t;

private:

    nameResolver_t globalResolver;
    localNameResolvers_t localResolvers;

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("name-tracker")
#include "Util/unmacros.h"

    NameTracker() : ModulePass(ID) {};
    virtual ~NameTracker() {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnModule(llvm::Module& M) override;
    virtual void print(llvm::raw_ostream& O , const llvm::Module* M) const override;

    const nameResolver_t& getGlobalResolver() {
        return globalResolver;
    }

    const nameResolver_t& getLocalResolver(llvm::Function* F) {
        return localResolvers[F];
    }

    llvm::Value* resolve(const std::string& name) {
        if (globalResolver.count(name)) return globalResolver[name];
        else return nullptr;
    }

    llvm::Value* resolve(const std::string& name, llvm::Function* context) {
        if (localResolvers.count(context) && localResolvers[context].count(name)) return localResolvers[context][name];
        else return nullptr;
    }
};

} // namespace borealis

#endif /* NAMETRACKER_H_ */
