/*
 * TassadarPass.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: belyaev
 */

#include <iostream>
#include <fstream>

#include <llvm/Pass.h>
#include <llvm/PassAnalysisSupport.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetLibraryInfo.h>

#include "Config/config.h"
#include "Executor/Executor.h"
#include "Util/passes.hpp"
#include "Util/collections.hpp"
#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {
namespace {

const std::string DumpOutputFileDefault = "tassadar.report";

using namespace borealis::config;
MultiConfigEntry functionsToRun {"executor", "function"};

class ZeroArbiter: public Arbiter {
public:
    virtual llvm::GenericValue map(llvm::Value* val) override {
        llvm::GenericValue RetVal;
        if(val->getType()->isIntegerTy()) {
            RetVal.IntVal = llvm::APInt{ val->getType()->getIntegerBitWidth(), 0 };
        } else if(val->getType()->isFloatTy()) {
            RetVal.FloatVal = 0.0f;
        } else if(val->getType()->isDoubleTy()) {
            RetVal.DoubleVal = 0.0;
        }
        UNREACHABLE("unsupported type");
    }
    virtual ~ZeroArbiter(){}
};

class TassadarPass : public llvm::ModulePass {
public:
    static char ID;
    TassadarPass(): llvm::ModulePass(ID) {};

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        AUX<llvm::DataLayoutPass>::addRequired(AU);
        AUX<llvm::TargetLibraryInfo>::addRequired(AU);

        AU.setPreservesAll();
    }

    bool runOnModule(llvm::Module& M) override {
        TRACE_FUNC;

        auto funcNames = util::viewContainer(functionsToRun).toHashSet();
        Executor tassadar{&M,
            &getAnalysis<llvm::DataLayoutPass>().getDataLayout(),
            &getAnalysis<llvm::TargetLibraryInfo>(),
            std::make_shared<ZeroArbiter>()
        };

        auto funcs = util::viewContainer(M)
                    .filter(LAM(F, funcNames.count(F.getName())))
                    .map(LAM(F, &F))
                    .toHashSet();

        static config::StringConfigEntry DumpOutputFileOpt("output", "tassadar-output-file");

        std::string pattern = DumpOutputFileOpt.get(DumpOutputFileDefault);
        std::ofstream ofs{pattern};
        ofs << "[" << std::endl;

        if(funcs.empty()) return false;

        try {
            tassadar.runFunction(util::head(funcs), {});
            ofs << "null";
        } catch(const std::exception& ex) {
            ofs << '"' << ex.what() << '"';
        }

        for(auto&& F : util::tail(funcs)) {
            try {
                tassadar.runFunction(F, {});
                ofs << ", null";
            } catch(const std::exception& ex) {
                ofs << ", " << '"' << ex.what() << '"';
            }
        }

        ofs << std::endl << "]" << std::endl;

        return false;
    }

};

char TassadarPass::ID = 87;
static RegisterPass<TassadarPass>
X("tassadar", "Run executor");

} /* namespace */
} /* namespace borealis */

#include "Util/unmacros.h"
