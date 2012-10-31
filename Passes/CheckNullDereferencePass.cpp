/*
 * CheckNullDereferencePass.cpp
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#include "CheckNullDereferencePass.h"

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Support/InstVisitor.h>

#include "lib/poolalloc/src/DSA/DataStructureAA.h"
#include "Query/NullPtrQuery.h"
#include "Solver/Z3Solver.h"

namespace borealis {

using util::for_each;
using util::streams::endl;

class DerefInstVisitor : public InstVisitor<DerefInstVisitor> {

public:

    DerefInstVisitor(CheckNullDereferencePass* pass) : pass(pass) {
    }

    void visitLoadInst(llvm::LoadInst& I) {
        using llvm::Value;

        const Value* ptr = I.getPointerOperand();
        if (ptr->isDereferenceablePointer()) return;

        for (const auto* derefNullValue : *(pass->DerefNullSet)) {
            if (pass->AA->alias(ptr, derefNullValue) != AliasAnalysis::AliasResult::NoAlias) {
                pass->ValueNullSet->insert(&I);
            }
        }
    }

private:

    CheckNullDereferencePass* pass;

};

class ValueInstVisitor : public InstVisitor<ValueInstVisitor> {

public:

    ValueInstVisitor(CheckNullDereferencePass* pass) : pass(pass) {
    }

    void visitLoadInst(llvm::LoadInst& I) {
        using llvm::Value;

        const Value* ptr = I.getPointerOperand();
        if (ptr->isDereferenceablePointer()) return;

        for (const auto* nullValue : *(pass->ValueNullSet)) {
            if (pass->AA->alias(ptr, nullValue) != AliasAnalysis::AliasResult::NoAlias) {
                if (checkNullDereference(I, *ptr)) {
                    reportNullDereference(I, *ptr, *nullValue);
                }
            }
        }
    }

    void visitStoreInst(llvm::StoreInst& I) {
        using llvm::Value;

        const Value* ptr = I.getPointerOperand();
        if (ptr->isDereferenceablePointer()) return;

        for (const auto nullValue : *(pass->ValueNullSet)) {
            if (pass->AA->alias(ptr, nullValue) != AliasAnalysis::AliasResult::NoAlias) {
                if (checkNullDereference(I, *ptr)) {
                    reportNullDereference(I, *ptr, *nullValue);
                }
            }
        }
    }

    bool checkNullDereference(
            const llvm::Instruction& where,
            const llvm::Value& what) {
        using namespace::z3;

        NullPtrQuery q = NullPtrQuery(&what, pass->slotTracker);

        PredicateStateVector psv = pass->PSA->getPredicateStateMap()[&where];
        for (const auto& ps : psv) {
            context ctx;
            Z3ExprFactory z3ef(ctx);
            Z3Solver s(z3ef);

            if (s.checkSatOrUnknown(q, ps)) {
                return true;
            }
        }

        return false;
    }

    void reportNullDereference(
            const llvm::Value& in,
            const llvm::Value& what,
            const llvm::Value& from) {
        llvm::errs() << "Possible NULL dereference in" << endl
                << "\t" << in << endl
                << "from" << endl
                << "\t" << from << endl
                << "with" << endl
                << "\t" << what << endl;
    }

private:

    CheckNullDereferencePass* pass;

};

CheckNullDereferencePass::CheckNullDereferencePass() : llvm::FunctionPass(ID) {
    // TODO
}

void CheckNullDereferencePass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
    using namespace::llvm;

    Info.setPreservesAll();
    Info.addRequiredTransitive<DSAA>();
    Info.addRequiredTransitive<DetectNullPass>();
    Info.addRequiredTransitive<PredicateStateAnalysis>();
    Info.addRequiredTransitive<SlotTrackerPass>();
}

bool CheckNullDereferencePass::runOnFunction(llvm::Function& F) {
    using namespace::std;
    using namespace::llvm;

    AA = &getAnalysis<DSAA>();
    DNP = &getAnalysis<DetectNullPass>();
    PSA = &getAnalysis<PredicateStateAnalysis>();
    slotTracker = getAnalysis<SlotTrackerPass>().getSlotTracker(F);

    auto valueSet = DNP->getNullSet(NullType::VALUE);
    auto derefSet = DNP->getNullSet(NullType::DEREF);

    ValueNullSet = &valueSet;
    DerefNullSet = &derefSet;

    DerefInstVisitor div(this);
    div.visit(F);

    ValueInstVisitor viv(this);
    viv.visit(F);

    return false;
}

CheckNullDereferencePass::~CheckNullDereferencePass() {
    // TODO
}

} /* namespace borealis */

char borealis::CheckNullDereferencePass::ID;
static llvm::RegisterPass<borealis::CheckNullDereferencePass>
X("check-null-deref", "NULL dereference checker", false, false);
