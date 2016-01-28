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

static std::pair<llvm::Value*,llvm::Value*> breakOverflowIntrinsic(llvm::IntrinsicInst* ii) {
    using namespace llvm;
    IRBuilder<> builder(ii->getParent(), ii);

    Value *op1 = ii->getArgOperand(0);
    Value *op2 = ii->getArgOperand(1);

    Value *result = 0;
    Value *result_ext = 0;
    Value *overflow = 0;

    unsigned int bw = op1->getType()->getPrimitiveSizeInBits();
    unsigned int bw2 = op1->getType()->getPrimitiveSizeInBits()*2;

    if ((ii->getIntrinsicID() == Intrinsic::uadd_with_overflow) ||
        (ii->getIntrinsicID() == Intrinsic::usub_with_overflow) ||
        (ii->getIntrinsicID() == Intrinsic::umul_with_overflow)) {

        Value *op1ext =
            builder.CreateZExt(op1, IntegerType::get(ii->getContext(), bw2));
        Value *op2ext =
            builder.CreateZExt(op2, IntegerType::get(ii->getContext(), bw2));
        Value *int_max_s =
            ConstantInt::get(op1->getType(), APInt::getMaxValue(bw));
        Value *int_max =
            builder.CreateZExt(int_max_s, IntegerType::get(ii->getContext(), bw2));

        if (ii->getIntrinsicID() == Intrinsic::uadd_with_overflow){
            result_ext = builder.CreateAdd(op1ext, op2ext);
        } else if (ii->getIntrinsicID() == Intrinsic::usub_with_overflow){
            result_ext = builder.CreateSub(op1ext, op2ext);
        } else if (ii->getIntrinsicID() == Intrinsic::umul_with_overflow){
            result_ext = builder.CreateMul(op1ext, op2ext);
        }
        overflow = builder.CreateICmpUGT(result_ext, int_max);

    } else if ((ii->getIntrinsicID() == Intrinsic::sadd_with_overflow) ||
               (ii->getIntrinsicID() == Intrinsic::ssub_with_overflow) ||
               (ii->getIntrinsicID() == Intrinsic::smul_with_overflow)) {

        Value *op1ext =
            builder.CreateSExt(op1, IntegerType::get(ii->getContext(), bw2));
        Value *op2ext =
            builder.CreateSExt(op2, IntegerType::get(ii->getContext(), bw2));
        Value *int_max_s =
            ConstantInt::get(op1->getType(), APInt::getSignedMaxValue(bw));
        Value *int_min_s =
            ConstantInt::get(op1->getType(), APInt::getSignedMinValue(bw));
        Value *int_max =
            builder.CreateSExt(int_max_s, IntegerType::get(ii->getContext(), bw2));
        Value *int_min =
            builder.CreateSExt(int_min_s, IntegerType::get(ii->getContext(), bw2));

        if (ii->getIntrinsicID() == Intrinsic::sadd_with_overflow){
            result_ext = builder.CreateAdd(op1ext, op2ext);
        } else if (ii->getIntrinsicID() == Intrinsic::ssub_with_overflow){
            result_ext = builder.CreateSub(op1ext, op2ext);
        } else if (ii->getIntrinsicID() == Intrinsic::smul_with_overflow){
            result_ext = builder.CreateMul(op1ext, op2ext);
        }
        overflow = builder.CreateOr(builder.CreateICmpSGT(result_ext, int_max),
                                    builder.CreateICmpSLT(result_ext, int_min));
    }

    // This trunc cound be replaced by a more general trunc replacement
    // that allows to detect also undefined behavior in assignments or
    // overflow in operation with integers whose dimension is smaller than
    // int's dimension, e.g.
    //     uint8_t = uint8_t + uint8_t;
    // if one desires the wrapping should write
    //     uint8_t = (uint8_t + uint8_t) & 0xFF;
    // before this, must check if it has side effects on other operations
    result = builder.CreateTrunc(result_ext, op1->getType());
    return { result, overflow };
};

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
        auto&& intrinsics = IntrinsicsManager::getInstance();
        LLVM_type_builder type_builder(&F);

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

        auto $SomeStructTy = functional_hell::matchers::Guard([](llvm::Type* t){ return t && t -> isStructTy(); });

        auto intr_stores =
            util::viewContainer(F)
            .flatten()
            .map(ops::take_pointer)
            .filter(llvm::pattern_matcher(llvm::$StoreInst(_, llvm::$CallInst(llvm::$Intrinsic(_), _) & llvm::$OfType($SomeStructTy))))
            .toVector();

        for(auto&& E : intr_stores) SWITCH(E) {
                NAMED_CASE(m,
                           llvm::$StoreInst(
                               _1,
                               _2
                               & llvm::$CallInst(llvm::$Intrinsic(_), _)
                               & llvm::$OfType($SomeStructTy)
                           )
                ) {
                    auto dst = m->_1;
                    auto call = m->_2;

                    auto resolves = breakOverflowIntrinsic(llvm::cast<llvm::IntrinsicInst>(call));
                    auto&& gep0 = llvm::GetElementPtrInst::Create(
                        dst,
                        util::itemize(0, 0).map(toLLVMConstant).toVector(),
                        dst->getName() + ".value",
                        E
                    );
                    auto&& gep1 = llvm::GetElementPtrInst::Create(
                        dst,
                        util::itemize(0, 1).map(toLLVMConstant).toVector(),
                        dst->getName() + ".overflow",
                        E
                    );

                    auto&& store0 = new llvm::StoreInst(gep0, resolves.first, E);
                    auto&& store1 = new llvm::StoreInst(gep1, resolves.second, E);
                    copyDbgMD(E, gep0);
                    copyDbgMD(E, gep1);
                    copyDbgMD(E, store0);
                    copyDbgMD(E, store1);

                    E->eraseFromParent();
                    eraseIfNotUsed(call);
                }
        }

        auto $LocalFunction = functional_hell::matchers::Guard([](llvm::Value* v){
            if(auto&& f = dyn_cast<llvm::Function>(v)) {
                return !f->isDeclaration();
            }
            return false;
        });

        auto func_stores =
            util::viewContainer(F)
            .flatten()
            .map(ops::take_pointer)
            .filter(llvm::pattern_matcher(llvm::$StoreInst(_, llvm::$CallInst($LocalFunction, _) & llvm::$OfType($SomeStructTy))))
            .toVector();

        for(auto&& E : func_stores) SWITCH(E) {
                NAMED_CASE(m,
                           llvm::$StoreInst(
                               _1,
                               llvm::$CallInst(_2 & $LocalFunction, _3)
                               & llvm::$OfType($SomeStructTy)
                           )
                ) {
                    auto dst = m->_1;
                    auto func = m->_2;
                    auto args = m->_3;

                    auto refinedArgs = util::itemize(dst, func) >> util::viewContainer(args);

                    auto callAndStore = intrinsics.createIntrinsic(
                        function_type::INTRINSIC_CALL_AND_STORE,
                        util::toString(*func->getType()),
                        type_builder.getFunction(type_builder.getVoid(), refinedArgs.map(LAM(arg, arg->getType())) ),
                        F.getParent()
                    );

                    auto call = llvm::CallInst::Create(callAndStore, refinedArgs.toVector(), "", E);
                    copyDbgMD(E, call);

                    eraseIfNotUsed(llvm::cast<llvm::StoreInst>(E)->getValueOperand());
                    E->eraseFromParent();
                }
        }

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
