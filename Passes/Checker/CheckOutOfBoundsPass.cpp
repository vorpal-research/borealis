/*
 * CheckOutOfBoundsPass.cpp
 *
 *  Created on: Sep 2, 2013
 *      Author: sam
 */

#include <llvm/Support/InstVisitor.h>

#include "Codegen/intrinsics_manager.h"
#include "Codegen/llvm.h"
#include "Passes/Checker/CheckOutOfBoundsPass.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "SMT/MathSAT/Solver.h"
#include "SMT/Z3/Solver.h"
#include "State/PredicateStateBuilder.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class AllocaInstVisitor : public llvm::InstVisitor<AllocaInstVisitor> {

public:

    AllocaInstVisitor(CheckOutOfBoundsPass* pass, llvm::ValueSymbolTable* vst)
                        : pass(pass), arrays_(), vst_(vst) {}

    void visitAllocaInst(llvm::AllocaInst &I) {
        if (I.getAllocatedType()->isArrayTy()) {
            auto value = vst_->lookup(I.getName());
            arrays_.push_back(value);
        }
    }

    std::vector<llvm::Value*> getArrays() {
        return arrays_;
    }


private:

    CheckOutOfBoundsPass* pass;
    std::vector<llvm::Value*> arrays_;
    llvm::ValueSymbolTable* vst_;

};

////////////////////////////////////////////////////////////////////////////////

class GepInstVisitor : public llvm::InstVisitor<GepInstVisitor> {

public:

    typedef std::pair<llvm::Type*, llvm::Value*> TypeValue;
    typedef std::vector<TypeValue> TypeValueVector;
    typedef std::pair<llvm::ArrayType*, llvm::Value*> ArrayValue;
    typedef std::vector<ArrayValue> ArrayValueVector;


    GepInstVisitor(CheckOutOfBoundsPass* pass, AllocaInstVisitor allocaVisitor)
                            : pass(pass), allocaVisitor_(allocaVisitor) {}


    void visitGetElementPtrInst(llvm::GetElementPtrInst &GI) {
        auto spliters = filterArrays(splitGep(GI));
        for (const auto& spliter: spliters) {
            auto numElements = spliter.first->getNumElements();
            if (checkOutOfBounds(GI, *spliter.second, numElements)) {
                reportOutOfBounds(GI);
            }
        }
    }


private:

    TypeValueVector splitGep(const llvm::GetElementPtrInst &GI) {
        using namespace llvm;
        auto ptrType = GI.getPointerOperand()->getType();

        TypeValueVector spliters;
        spliters.reserve(GI.getNumIndices());
        spliters.emplace_back(ptrType, GI.getOperand(1));

        std::vector<llvm::Value *> indicies;
        indicies.reserve(GI.getNumIndices());
        auto endIter = GI.idx_end();
        auto opIter = GI.idx_begin();
        ++opIter;
        for (; opIter != endIter; ++opIter) {
            auto type = GetElementPtrInst::getIndexedType(ptrType, indicies);
            spliters.emplace_back(type, *opIter);
            indicies.push_back(*opIter);
        }
        return spliters;
    }


    llvm::ArrayType* haveArrayAlias(const TypeValue& value,
                                        const std::vector<llvm::Value*> arrays) {
        using llvm::AliasAnalysis;
        using llvm::GetElementPtrInst;
        for (const auto& array: arrays) {
            auto aliasRes = pass->AA->alias(array, value.second);
            if (aliasRes == AliasAnalysis::AliasResult::MustAlias) {
                auto pointedType = GetElementPtrInst::getIndexedType(array->getType(),
                                                    std::vector<llvm::Constant*>());
                auto arrType = llvm::dyn_cast<llvm::ArrayType>(pointedType);
                return arrType;
            }
        }
        return nullptr;
    }


    ArrayValueVector filterArrays(
            const TypeValueVector &splitters) {

        auto arrays = allocaVisitor_.getArrays();

        ArrayValueVector filtered;
        for (const auto& splitter: splitters) {
            if (splitter.first->isArrayTy()) {
                auto arrType = llvm::dyn_cast<llvm::ArrayType>(splitter.first);
                filtered.emplace_back(arrType, splitter.second);
            } else if(splitter.first->isPointerTy()) {
                auto aliasType = haveArrayAlias(splitter, arrays);
                if (aliasType != nullptr) {
                    filtered.emplace_back(aliasType, splitter.second);
                    continue;
                }
                aliasType = haveArrayAlias(splitter, pass->globalArrays_);
                if (aliasType != nullptr) {
                    filtered.emplace_back(aliasType, splitter.second);
                    continue;
                }
            }
        }
        return filtered;
    }


    bool checkOutOfBounds(
            llvm::Instruction& where,
            llvm::Value& what,
            unsigned howMuch) {


        PredicateState::Ptr q = (
            pass->FN.State *
            pass->FN.Predicate->getEqualityPredicate(
                pass->FN.Term->getCmpTerm(llvm::ConditionType::LT,
                                          pass->FN.Term->getValueTerm(&what),
                                          pass->FN.Term->getIntTerm(howMuch)
                ),
                pass->FN.Term->getTrueTerm()
            )
        )();

        PredicateState::Ptr ps = pass->PSA->getInstructionState(&where);

        dbgs() << "Query: " << q->toString() << endl;
        dbgs() << "State: " << ps << endl;

#if defined USE_MATHSAT_SOLVER
        MathSAT::ExprFactory ef;
        MathSAT::Solver s(ef);
#else
        Z3::ExprFactory ef;
        Z3::Solver s(ef);
#endif

        if (s.isViolated(q, ps)) {
            dbgs() << "Violated!" << endl;
            return true;
        } else {
            dbgs() << "Passed!" << endl;
            return false;
        }
    }

    void reportOutOfBounds(llvm::Instruction& where) {
        pass->DM->addDefect(DefectType::BUF_05, &where);
    }


private:
    CheckOutOfBoundsPass* pass;
    AllocaInstVisitor allocaVisitor_;

};

////////////////////////////////////////////////////////////////////////////////

CheckOutOfBoundsPass::CheckOutOfBoundsPass() : ProxyFunctionPass(ID), globalArrays_() {}
CheckOutOfBoundsPass::CheckOutOfBoundsPass(llvm::Pass* pass) :
        ProxyFunctionPass(ID, pass),
        globalArrays_() {}

bool CheckOutOfBoundsPass::doInitialization(llvm::Module& module) {
    std::cout << "hi" << std::endl;
    auto end = module.getGlobalList().end();
    for ( auto iter = module.getGlobalList().begin(); iter != end; ++iter) {
        auto type = (*iter).getType();
        auto pointedType = llvm::GetElementPtrInst::getIndexedType(type, std::vector<llvm::Constant*>());
        if (pointedType->isArrayTy()) {
            globalArrays_.push_back(&*iter);
        }
    }
    return false;
}

void CheckOutOfBoundsPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<llvm::AliasAnalysis>::addRequiredTransitive(AU);
    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool CheckOutOfBoundsPass::runOnFunction(llvm::Function& F) {
    for (auto global: globalArrays_) {
        std::cout << *global->getType() << " : " << *global << std::endl;
    }

    AA = &GetAnalysis<llvm::AliasAnalysis>::doit(this, F);

    DM = &GetAnalysis<DefectManager>::doit(this, F);
    PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(st);

    AllocaInstVisitor aiv(this, &F.getValueSymbolTable());
    aiv.visit(F);
    GepInstVisitor giv(this, aiv);
    giv.visit(F);
    return false;
}

CheckOutOfBoundsPass::~CheckOutOfBoundsPass() {}

char CheckOutOfBoundsPass::ID;
static RegisterPass<CheckOutOfBoundsPass>
X("check-bounds", "Pass that checks out of bounds");

} /* namespace borealis */
