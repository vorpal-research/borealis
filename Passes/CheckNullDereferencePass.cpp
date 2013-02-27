/*
 * CheckNullDereferencePass.cpp
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Support/InstVisitor.h>

#include "lib/poolalloc/src/DSA/DataStructureAA.h"

#include "Logging/logger.hpp"
#include "Passes/CheckNullDereferencePass.h"
#include "Passes/PredicateStateAnalysis.h"
#include "Solver/Z3Solver.h"
#include "Util/passes.hpp"

namespace borealis {

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

class ValueInstVisitor :
    public llvm::InstVisitor<ValueInstVisitor> {

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

        Predicate::Ptr q = pass->PF->getInequalityPredicate(
                pass->TF->getValueTerm(&what),
                pass->TF->getNullPtrTerm());

        pass->infos() << "Query: " << q->toString() << endl;

        PredicateStateVector psv = pass->PSA->getPredicateStateMap()[&where];
        for (const auto& ps : psv) {

            pass->infos() << "Checking state: " << ps << endl;

            if (!ps.hasVisited(where, what, why)) {

                pass->infos() << "Infeasible!" << endl;

                continue;
            }

            z3::context ctx;
            Z3ExprFactory z3ef(ctx);
            Z3Solver s(z3ef);

            if (s.checkSatOrUnknown(q, ps.filter())) {
                continue;
            } else {
                pass->infos() << "Violated!" << endl;
                return true;
            }
        }

        pass->infos() << "Passed!" << endl;
        return false;
    }

    void reportNullDereference(
            llvm::Instruction& where,
            llvm::Value& /* what */,
            llvm::Value& /* from */) {
        pass->defectManager->addDefect(DefectType::INI_03, &where);
    }

private:

    CheckNullDereferencePass* pass;

};

CheckNullDereferencePass::CheckNullDereferencePass() : ProxyFunctionPass(ID) {}
CheckNullDereferencePass::CheckNullDereferencePass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void CheckNullDereferencePass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    using namespace::llvm;

    AU.setPreservesAll();

    AUX<AliasAnalysis>::addRequiredTransitive(AU);
    AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    AUX<DetectNullPass>::addRequiredTransitive(AU);
    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool CheckNullDereferencePass::runOnFunction(llvm::Function& F) {
    using namespace::llvm;

    AA = &GetAnalysis<AliasAnalysis>::doit(this, F);

    PSA = &GetAnalysis<PredicateStateAnalysis>::doit(this, F);
    DNP = &GetAnalysis<DetectNullPass>::doit(this, F);

    defectManager = &GetAnalysis<DefectManager>::doit(this, F);
    slotTracker = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);

    PF = PredicateFactory::get(slotTracker);
    TF = TermFactory::get(slotTracker);

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

void CheckNullDereferencePass::print(llvm::raw_ostream& s, const llvm::Module*) const {
    using borealis::util::streams::endl;
    s << "DerefNullSet:" << endl << *DerefNullSet << endl;
    s << "ValueNullSet:" << endl << *ValueNullSet << endl;
}

CheckNullDereferencePass::~CheckNullDereferencePass() {}

char CheckNullDereferencePass::ID;
static RegisterPass<CheckNullDereferencePass>
X("check-null-deref", "NULL dereference checker");

} /* namespace borealis */
