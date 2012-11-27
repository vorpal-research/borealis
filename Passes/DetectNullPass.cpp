/*
 * DetectNullPass.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: ice-phoenix
 */

#include "DetectNullPass.h"

#include <llvm/Constants.h>
#include <llvm/Support/InstVisitor.h>

#include "Logging/tracer.hpp"

namespace borealis {

using namespace borealis::util;

////////////////////////////////////////////////////////////////////////////////
//
// Regular instruction visitor
//
////////////////////////////////////////////////////////////////////////////////

class RegularDetectNullInstVisitor :
        public llvm::InstVisitor<RegularDetectNullInstVisitor> {

public:

    using llvm::InstVisitor<RegularDetectNullInstVisitor>::visit;

    RegularDetectNullInstVisitor(DetectNullPass* pass) : pass(pass) {}

    void visit(llvm::Instruction& I) {
        using llvm::Value;

        Value* ptr = &I;
        if (containsKey(pass->data, ptr)) return;

        InstVisitor::visit(I);
    }

    void visitCallInst(llvm::CallInst& I) {
        if (!I.getType()->isPointerTy()) return;

        pass->data[&I] = NullInfo().setStatus(NullStatus::Maybe_Null);
    }

    void visitStoreInst(llvm::StoreInst& I) {
        using namespace llvm;

        Value* ptr = I.getPointerOperand();
        Value* value = I.getValueOperand();

        if (!ptr->getType()->getPointerElementType()->isPointerTy()) return;

        if (isa<ConstantPointerNull>(*value)) {
            pass->data[ptr] = NullInfo().setType(NullType::DEREF).setStatus(NullStatus::Null);
        }
    }

    void visitInsertValueInst(llvm::InsertValueInst& I) {
        using namespace llvm;

        Value* value = I.getInsertedValueOperand();
        if (isa<ConstantPointerNull>(value)) {
            const std::vector<unsigned> idxs = I.getIndices().vec();
            pass->data[&I] = NullInfo().setStatus(idxs, NullStatus::Null);
        }
    }

private:

    DetectNullPass* pass;

};

////////////////////////////////////////////////////////////////////////////////
//
// PHINode instruction visitor
//
////////////////////////////////////////////////////////////////////////////////

class PHIDetectNullInstVisitor :
        public llvm::InstVisitor<PHIDetectNullInstVisitor> {

public:

    PHIDetectNullInstVisitor(DetectNullPass* pass) : pass(pass) {}

    void visitPHINode(llvm::PHINode& I) {
        using namespace llvm;

        if (!I.getType()->isPointerTy()) return;

        std::set<PHINode*> visited;
        auto incomingValues = getIncomingValues(I, visited);

        NullInfo nullInfo = NullInfo();
        for (Value* II : incomingValues) {
            if (containsKey(pass->data, II)) {
                nullInfo = nullInfo.merge(pass->data[II]);
            } else {
                nullInfo = nullInfo.merge(NullStatus::Not_Null);
            }
        }
        pass->data[&I] = nullInfo;
    }

private:

    DetectNullPass* pass;

    std::set<llvm::Value*> getIncomingValues(
            llvm::PHINode& I,
            std::set<llvm::PHINode*>& visited) {
        using namespace llvm;

        std::set<Value*> res;

        if (contains(visited, &I)) return res;
        else visited.insert(&I);

        for (unsigned i = 0U; i < I.getNumIncomingValues(); ++i) {
            Value* incoming = I.getIncomingValue(i);
            if (isa<Value>(incoming)) {
                if (isa<PHINode>(incoming)) {
                    auto sub = getIncomingValues(
                            *cast<PHINode>(incoming),
                            visited);
                    res.insert(sub.begin(), sub.end());
                } else {
                    res.insert(cast<Value>(incoming));
                }
            }
        }

        return res;
    }
};

////////////////////////////////////////////////////////////////////////////////

DetectNullPass::DetectNullPass() : llvm::FunctionPass(ID) {}

void DetectNullPass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	Info.setPreservesAll();
}

bool DetectNullPass::runOnFunction(llvm::Function& F) {
    TRACE_FUNC;

	init();

	RegularDetectNullInstVisitor regularVisitor(this);
	regularVisitor.visit(F);

	PHIDetectNullInstVisitor phiVisitor(this);
	phiVisitor.visit(F);

	return false;
}

DetectNullPass::~DetectNullPass() {}

llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const NullInfo& info) {
	using borealis::util::enums::asInteger;
	using borealis::util::streams::endl;

    s << asInteger(info.type) << ":" << endl;
	for(const auto& entry : info.offsetInfoMap) {
		s << entry.first << "->" << asInteger(entry.second) << endl;
	}

	return s;
}

char DetectNullPass::ID;
static llvm::RegisterPass<DetectNullPass>
X("detect-null", "Explicit NULL assignment detector");

} /* namespace borealis */
