/*
 * CheckNullDereferencePass.cpp
 *
 *  Created on: Aug 24, 2012
 *      Author: ice-phoenix
 */

#include "CheckNullDereferencePass.h"

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>

#include "../lib/poolalloc/src/DSA/DataStructureAA.h"
#include "../Query/NullPtrQuery.h"
#include "../Solver/util.h"

namespace borealis {

using util::for_each;
using util::streams::endl;

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
	st = getAnalysis<SlotTrackerPass>().getSlotTracker(F);

	auto tmp = DNP->getNullSet();
	NullSet = &tmp;

	for_each(F, [this](const BasicBlock& BB){
		for_each(BB, [this](const Instruction& I){
			processInst(I);
		});
	});

	return false;
}

void CheckNullDereferencePass::processInst(const llvm::Instruction& I) {
	using namespace::llvm;

	if (isa<LoadInst>(I))
		{ process(cast<LoadInst>(I)); }

	else if (isa<StoreInst>(I))
		{ process(cast<StoreInst>(I)); }
}

void CheckNullDereferencePass::process(const llvm::LoadInst& I) {
	using namespace::std;
	using namespace::llvm;

	const Value* ptr = I.getPointerOperand();

	if (ptr->isDereferenceablePointer()) return;

	for_each(*NullSet, [this, &I, ptr](const Value* nullValue){
		if (AA->alias(ptr, nullValue) != AliasAnalysis::AliasResult::NoAlias) {
		    if (checkNullDereference(I, *ptr)) {
                reportNullDereference(I, *nullValue);
            }
		}
	});
}

void CheckNullDereferencePass::process(const llvm::StoreInst& I) {
	using namespace::std;
	using namespace::llvm;

	const Value* ptr = I.getPointerOperand();

	if (ptr->isDereferenceablePointer()) return;

	for_each(*NullSet, [this, &I, ptr](const Value* nullValue){
		if (AA->alias(ptr, nullValue) != AliasAnalysis::AliasResult::NoAlias) {
		    if (checkNullDereference(I, *ptr)) {
		        reportNullDereference(I, *nullValue);
		    }
		}
	});
}

void CheckNullDereferencePass::reportNullDereference(
		const llvm::Value& in,
		const llvm::Value& from) {
	using namespace::llvm;

	errs() << "Possible NULL dereference in" << endl
			<< "\t" << in << endl
			<< "from" << endl
			<< "\t" << from << endl;
}

bool CheckNullDereferencePass::checkNullDereference(
        const llvm::Instruction& where,
        const llvm::Value& what) {
    using namespace::std;
    using namespace::llvm;
    using namespace::z3;

    NullPtrQuery q = NullPtrQuery(&what, st);

    PredicateStateVector psv = PSA->getPredicateStateMap()[&where];
    for (const auto& ps : psv) {
        context ctx;

        expr assertion = q.toZ3(ctx);
        pair<expr, expr> state = ps.toZ3(ctx);

        if (
                checkSatOrUnknown(
                        assertion,
                        vector<expr> { state.first, state.second },
                        ctx)
        ) {
            return true;
        }
    }

    return false;
}

CheckNullDereferencePass::~CheckNullDereferencePass() {
	// TODO
}

} /* namespace borealis */

char borealis::CheckNullDereferencePass::ID;
static llvm::RegisterPass<borealis::CheckNullDereferencePass>
X("check-null-deref", "NULL dereference checker", false, false);
