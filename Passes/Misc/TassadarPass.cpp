/*
 * TassadarPass.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: belyaev
 */
#include <iostream>
#include <fstream>
#include <random>

#include <llvm/Pass.h>
#include <llvm/PassAnalysisSupport.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetLibraryInfo.h>

#include "Config/config.h"
#include "Executor/ExecutionEngine.h"
#include "Util/passes.hpp"
#include "Util/collections.hpp"
#include "Util/functional.hpp"
#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {
namespace {

const std::string DumpOutputFileDefault = "tassadar.report";

using namespace borealis::config;
MultiConfigEntry functionsToRun {"executor", "function"};
StringConfigEntry arbiterMode{"executor", "defaultArbiter"};

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
        } else if(val->getType()->isPointerTy()) {
            RetVal.PointerVal = nullptr;
        } else UNREACHABLE("unsupported type: " + util::toString(*val->getType()));

        return RetVal;
    }
    virtual ~ZeroArbiter(){}
};

class RandomArbiter: public Arbiter {
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;

public:
    RandomArbiter(): rd(), gen(rd()), dis(0.0, 8000000.0) {}

    virtual llvm::GenericValue map(llvm::Value* val) override {
        llvm::GenericValue RetVal;
        if(val->getType()->isIntegerTy()) {
            RetVal.IntVal = llvm::APInt{ val->getType()->getIntegerBitWidth(), gen()  };
        } else if(val->getType()->isFloatTy()) {
            RetVal.FloatVal = static_cast<float>(dis(gen));
        } else if(val->getType()->isDoubleTy()) {
            RetVal.DoubleVal = dis(gen);
        } else if(val->getType()->isPointerTy()) {
            RetVal.PointerVal = nullptr;
        } else UNREACHABLE("unsupported type: " + util::toString(*val->getType()));

        return RetVal;
    }
    virtual ~RandomArbiter(){}
};

class TassadarPass : public llvm::ModulePass {
public:
    static char ID;
    TassadarPass(): llvm::ModulePass(ID) {};

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        AUX<llvm::DataLayoutPass>::addRequired(AU);
        AUX<llvm::TargetLibraryInfo>::addRequired(AU);
        AUX<VariableInfoTracker>::addRequired(AU);
        AUX<SlotTrackerPass>::addRequired(AU);

        AU.setPreservesAll();
    }

    bool runOnModule(llvm::Module& M) override {
        TRACE_FUNC;

        auto funcNames = util::viewContainer(functionsToRun).toHashSet();
        auto arbiterName = arbiterMode.get("zero");
        using arbiter_ptr = std::shared_ptr<Arbiter>;
        auto arbiter =
            ("zero" == arbiterName) ? arbiter_ptr(std::make_shared<ZeroArbiter>()) :
            ("random" == arbiterName) ? arbiter_ptr(std::make_shared<RandomArbiter>()) :
            nullptr;
        ASSERT(nullptr != arbiter, tfm::format("Unknown arbiter mode: %s", arbiterName));

        ExecutionEngine tassadar{&M,
            &getAnalysis<llvm::DataLayoutPass>().getDataLayout(),
            &getAnalysis<llvm::TargetLibraryInfo>(),
            &getAnalysis<SlotTrackerPass>(),
            &getAnalysis<VariableInfoTracker>(),
            arbiter
        };

        auto funcs = util::viewContainer(M)
                    .filter(LAM(F, funcNames.count(F.getName())))
                    .map(ops::take_pointer)
                    .toHashSet();

        static config::StringConfigEntry DumpOutputFileOpt("output", "tassadar-output-file");

        std::string pattern = DumpOutputFileOpt.get(DumpOutputFileDefault);
        std::ofstream ofs{pattern};

        if(funcs.empty()) return false;

        ofs << "[" << std::endl;
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
