//
// Created by ice-phoenix on 10/13/15.
//

#include <llvm/Transforms/Utils/Local.h>

#include "Codegen/intrinsics_manager.h"
#include "Config/config.h"
#include "Logging/logger.hpp"
#include "Util/passes.hpp"
#include "Util/functional.hpp"
#include "Util/util.h"

#include "Util/llvm_matchers.hpp"

#include "Util/macros.h"

namespace borealis {

static void copyDbgMD(llvm::Instruction* from, llvm::Instruction* to) {
    if(from->getMetadata("dbg")) to->setMetadata("dbg", from->getMetadata("dbg"));
}

static void eraseIfNotUsed(llvm::Value* v) {
    if(auto&& i = dyn_cast<llvm::Instruction>(v)) {
        if(i->getNumUses() == 0) i->eraseFromParent();
    }
}

class KillStructByValue : public llvm::FunctionPass {

public:

    static char ID;

    KillStructByValue() : llvm::FunctionPass(ID) {}
    virtual ~KillStructByValue() {};

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        // Does not preserve CFG
        AU.setPreservesCFG();
    }

    virtual bool runOnFunction(llvm::Function& F) override {
        auto&& i32 = llvm::Type::getInt32Ty(F.getContext());

        auto toLLVMConstant = [&](auto&& i) -> llvm::Value* {
            return llvm::ConstantInt::get(i32, i);
        };

        auto toReplace =
            util::viewContainer(F)
            .flatten()
            .filter(LAM(I, I.getType()->isStructTy()))
            .map(ops::take_pointer)
            .toVector();

        for(auto&& A: toReplace) {
            llvm::DemoteRegToStack(*A);
        }

#include <functional-hell/matchers_fancy_syntax.h>

        using namespace functional_hell::matchers::placeholders;
        // Extract(Load($ptr), $indices) -> Load(Gep($ptr, $indices))

        auto extracts =
            util::viewContainer(F)
            .flatten()
            .map(ops::take_pointer)
            .filter(llvm::pattern_matcher(llvm::$ExtractValueInst(llvm::$LoadInst(_), _)))
            .toVector();

        for(auto&& E : extracts) SWITCH(E) {
                NAMED_CASE(m, llvm::$ExtractValueInst(_3 & llvm::$LoadInst(_1), _2)) {
                    auto ptr = m->_1;
                    auto param = m->_3;
                    auto indices = (util::itemize(0) >> util::viewContainer(m->_2))
                                  .map(toLLVMConstant)
                                  .toVector();

                    auto&& gep = llvm::GetElementPtrInst::Create(ptr, indices, E->getName() + ".ptr", E);
                    auto&& load = new llvm::LoadInst(gep, E->getName(), false, E);

                    copyDbgMD(E, gep);
                    copyDbgMD(E, load);

                    E->replaceAllUsesWith(load);
                    E->eraseFromParent();

                    eraseIfNotUsed(param);
                }
            }

        // Store($ptr, InsertValue(Load($ptr), $indices, $value)) -> Store(Gep($ptr, $indices), $value)

        auto inserts =
            util::viewContainer(F)
            .flatten()
            .map(ops::take_pointer)
            .filter(llvm::pattern_matcher(llvm::$StoreInst(_1, llvm::$InsertValueInst(llvm::$LoadInst(_1), _, _))))
            .toVector();


        for(auto&& E : inserts) SWITCH(E) {
            NAMED_CASE(
                m,
                llvm::$StoreInst(_1, llvm::$InsertValueInst(llvm::$LoadInst(_1), _2, _3))
            ) {
                auto indices = (util::itemize(0) >> util::viewContainer(m->_3))
                              .map(toLLVMConstant)
                              .toVector();
                auto ptr = m->_1;
                auto value = m->_2;

                auto insert = cast<llvm::StoreInst>(E)->getValueOperand();
                auto load = cast<llvm::InsertValueInst>(insert)->getAggregateOperand();

                auto&& gep = llvm::GetElementPtrInst::Create(ptr, indices, E->getName() + ".ptr", E);
                auto&& newStore = new llvm::StoreInst(gep, value, false, E);

                copyDbgMD(E, newStore);
                copyDbgMD(E, gep);

                E->replaceAllUsesWith(newStore);
                E->eraseFromParent();

                eraseIfNotUsed(insert);
                eraseIfNotUsed(load);
            }
        }

#include <functional-hell/matchers_fancy_syntax_off.h>

        return not toReplace.empty();
    }

};

char KillStructByValue::ID;
static RegisterPass<KillStructByValue>
    X("kill-struct-values", "Erase insertValue/extractValue instructions in code");

} /* namespace borealis */

#include "Util/unmacros.h"
