//
// Created by ice-phoenix on 10/13/15.
//

#include "Codegen/intrinsics_manager.h"
#include "Config/config.h"
#include "Logging/logger.hpp"
#include "Util/passes.hpp"
#include "Util/functional.hpp"
#include "Util/util.h"

namespace borealis {

class LoopBreaker : public llvm::FunctionPass {

public:

    static char ID;

    LoopBreaker() : llvm::FunctionPass(ID) {}
    virtual ~LoopBreaker() {};

    virtual void getAnalysisUsage(llvm::AnalysisUsage&) const override {
        // Does not preserve CFG
    }

    virtual bool runOnFunction(llvm::Function& F) override {

        auto&& im = IntrinsicsManager::getInstance();

        auto&& result = false;

        for (auto&& BB : F) {
            if (util::viewContainer(BB)
                    .map(ops::take_pointer)
                    .map(llvm::dyn_caster<llvm::CallInst>())
                    .filter()
                    .any_of([&](auto&& ci) { return function_type::INTRINSIC_UNREACHABLE == im.getIntrinsicType(*ci); })
            ) {
                auto&& term = BB.getTerminator();
                new llvm::UnreachableInst(term->getContext(), term);
                term->eraseFromParent();

                result = true;
            }
        }

        return result;
    }

};

char LoopBreaker::ID;
static RegisterPass<LoopBreaker>
    X("loop-breaker", "Break loops that must be broken");

} /* namespace borealis */
