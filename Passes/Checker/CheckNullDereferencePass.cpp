/*
 * CheckNullDereferencePass.cpp
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Support/InstVisitor.h>

#include "lib/poolalloc/src/DSA/DataStructureAA.h"

#include "Passes/Checker/CheckNullDereferencePass.h"
#include "SMT/Z3/Solver.h"
#include "SMT/MathSAT/Solver.h"
#include "SMT/MathSAT/Unlogic/Unlogic.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/TermRebinder.h"

namespace borealis {

////////////////////////////////////////////////////////////////////////////////

class DerefInstVisitor : public llvm::InstVisitor<DerefInstVisitor> {

public:

    DerefInstVisitor(CheckNullDereferencePass* pass) : pass(pass) {}

    void visitLoadInst(llvm::LoadInst& I) {
        using llvm::AliasAnalysis;
        using llvm::Value;

        Value* ptr = I.getPointerOperand();
        if (ptr->isDereferenceablePointer()) return;

        for (auto* derefNullValue : *(pass->DerefNullSet)) {
            if (pass->AA->alias(ptr, derefNullValue) != AliasAnalysis::AliasResult::NoAlias) {
                pass->ValueNullSet->insert(&I);
                break;
            }
        }
    }

private:

    CheckNullDereferencePass* pass;

};

////////////////////////////////////////////////////////////////////////////////

class ValueInstVisitor :
    public llvm::InstVisitor<ValueInstVisitor>,
    // this is by design (i.e., sharing logging facilities)
    public borealis::logging::ClassLevelLogging<CheckNullDereferencePass> {

public:

    ValueInstVisitor(CheckNullDereferencePass* pass) : pass(pass) {}

    void visitLoadInst(llvm::LoadInst& I) {
        using llvm::AliasAnalysis;
        using llvm::Value;

        Value* ptr = I.getPointerOperand();
        if (ptr->isDereferenceablePointer()) return;

        for (auto* nullValue : *(pass->ValueNullSet)) {
            if (pass->AA->alias(ptr, nullValue) != AliasAnalysis::AliasResult::NoAlias) {
                auto res = checkNullDereference(I, *ptr, *nullValue);
                if (res.first) {
                    auto di = pass->DM->getDefect(DefectType::INI_03, &I);
                    pass->DM->addDefect(di);
                    // pass->FM->addBond(I.getParent()->getParent(), {res.second, di});
                    break;
                }
            }
        }
    }

    void visitStoreInst(llvm::StoreInst& I) {
        using llvm::AliasAnalysis;
        using llvm::Value;

        Value* ptr = I.getPointerOperand();
        if (ptr->isDereferenceablePointer()) return;

        for (auto* nullValue : *(pass->ValueNullSet)) {
            if (pass->AA->alias(ptr, nullValue) != AliasAnalysis::AliasResult::NoAlias) {
                auto res = checkNullDereference(I, *ptr, *nullValue);
                if (res.first) {
                    auto di = pass->DM->getDefect(DefectType::INI_03, &I);
                    pass->DM->addDefect(di);
                    // pass->FM->addBond(I.getParent()->getParent(), {res.second, di});
                    break;
                }
            }
        }
    }

    std::pair<bool, PredicateState::Ptr> checkNullDereference(
            llvm::Instruction& where,
            llvm::Value& what,
            llvm::Value& why) {

        using borealis::mathsat_::unlogic::undoThat;
        using borealis::util::view;

        dbgs() << "Checking: " << endl
               << "  ptr: " << what << endl
               << "  aliasing: " << why << endl
               << "  at: " << where << endl;

        PredicateState::Ptr q = (
            pass->FN.State *
            pass->FN.Predicate->getInequalityPredicate(
                pass->FN.Term->getValueTerm(&what),
                pass->FN.Term->getNullPtrTerm()
            )
        )();

        PredicateState::Ptr ps = pass->PSA->getInstructionState(&where);

        if (!ps || !ps->hasVisited({&where, &what, &why})) {
            dbgs() << "Infeasible!" << endl;
            return {false, nullptr};
        }

        dbgs() << "Query: " << q << endl;
        dbgs() << "State: " << ps << endl;

        auto fMemId = pass->FM->getMemoryStart(where.getParent()->getParent());

#if defined USE_MATHSAT_SOLVER
        MathSAT::ExprFactory ef;
        MathSAT::Solver s(ef, fMemId);
#else
        Z3::ExprFactory ef;
        Z3::Solver s(ef, fMemId);
#endif

        if (s.isViolated(q, ps)) {
            dbgs() << "Violated!" << endl;

//            MathSAT::ExprFactory cef;
//            MathSAT::Solver cs(cef, fMemId);
//
//            auto& F = *where.getParent()->getParent();
//
//            auto args =
//                view(F.arg_begin(), F.arg_end())
//                .map([this](llvm::Argument& arg) { return pass->FN.Term->getArgumentTerm(&arg); })
//                .toVector();
//
//            auto c = cs.getContract(args, q, ps);
//            auto t = TermRebinder(F, pass->NT, pass->FN);
//            auto contract = undoThat(c)->map(
//                [&t](Predicate::Ptr p) { return t.transform(p); }
//            );

            return {true, pass->FN.State->Basic()};
        } else {
            dbgs() << "Passed!" << endl;
            return {false, nullptr};
        }
    }

private:

    CheckNullDereferencePass* pass;

};

////////////////////////////////////////////////////////////////////////////////

CheckNullDereferencePass::CheckNullDereferencePass() : ProxyFunctionPass(ID) {}
CheckNullDereferencePass::CheckNullDereferencePass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void CheckNullDereferencePass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<llvm::AliasAnalysis>::addRequiredTransitive(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<DetectNullPass>::addRequiredTransitive(AU);
    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<NameTracker>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool CheckNullDereferencePass::runOnFunction(llvm::Function& F) {

    AA = &GetAnalysis<llvm::AliasAnalysis>::doit(this, F);

    PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);
    DNP = &GetAnalysis<DetectNullPass>::doit(this, F);

    DM = &GetAnalysis<DefectManager>::doit(this, F);
    FM = &GetAnalysis<FunctionManager>::doit(this, F);
    NT = &GetAnalysis<NameTracker>::doit(this, F);

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(st);

    auto valueSet = DNP->getNullSet(NullType::VALUE);
    auto derefSet = DNP->getNullSet(NullType::DEREF);

    ValueNullSet = &valueSet;
    DerefNullSet = &derefSet;

    dbgs() << "DerefNullSet:" << endl << *DerefNullSet << endl;
    dbgs() << "ValueNullSet:" << endl << *ValueNullSet << endl;

    DerefInstVisitor div(this);
    div.visit(F);

    ValueInstVisitor viv(this);
    viv.visit(F);

    return false;
}

CheckNullDereferencePass::~CheckNullDereferencePass() {}

char CheckNullDereferencePass::ID;
static RegisterPass<CheckNullDereferencePass>
X("check-null-deref", "NULL dereference checker");

} /* namespace borealis */
