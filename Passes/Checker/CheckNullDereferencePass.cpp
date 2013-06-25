/*
 * CheckNullDereferencePass.cpp
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Support/InstVisitor.h>

#include "lib/poolalloc/src/DSA/DataStructureAA.h"

#include "Passes/Checker/CheckNullDereferencePass.h"
#include "Solver/Z3Solver.h"
#include "State/PredicateStateBuilder.h"

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
                if (checkNullDereference(I, *ptr, *nullValue)) {
                    reportNullDereference(I, *ptr, *nullValue);
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
                if (checkNullDereference(I, *ptr, *nullValue)) {
                    reportNullDereference(I, *ptr, *nullValue);
                    break;
                }
            }
        }
    }

    bool checkNullDereference(
            llvm::Instruction& where,
            llvm::Value& what,
            llvm::Value& why) {

        using llvm::dyn_cast;
        using llvm::Instruction;

        dbgs() << "Checking: " << endl
               << "  ptr: " << what << endl
               << "  aliasing: " << why << endl
               << "  at: " << where << endl;

        PredicateState::Ptr q = (
            pass->PSF *
            pass->PF->getInequalityPredicate(
                pass->TF->getValueTerm(&what),
                pass->TF->getNullPtrTerm()
            )
        )();

        PredicateState::Ptr ps = pass->PSA->getInstructionState(&where);

        if (!ps || !ps->hasVisited({&where, &what, &why})) {
            dbgs() << "Infeasible!" << endl;
            return false;
        }

        dbgs() << "Query: " << q->toString() << endl;
        dbgs() << "State: " << ps << endl;

        Z3ExprFactory z3ef;
        Z3Solver s(z3ef);

        if (s.isViolated(q, ps)) {
            dbgs() << "Violated!" << endl;
            return true;
        } else {
            dbgs() << "Passed!" << endl;
            return false;
        }
    }

    void reportNullDereference(
            llvm::Instruction& where,
            llvm::Value& /* what */,
            llvm::Value& /* from */) {
        pass->DM->addDefect(DefectType::INI_03, &where);
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
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool CheckNullDereferencePass::runOnFunction(llvm::Function& F) {

    AA = &GetAnalysis<llvm::AliasAnalysis>::doit(this, F);

    PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);
    DNP = &GetAnalysis<DetectNullPass>::doit(this, F);

    DM = &GetAnalysis<DefectManager>::doit(this, F);
    ST = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);

    PF = PredicateFactory::get(ST);
    TF = TermFactory::get(ST);

    PSF = PredicateStateFactory::get();

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
