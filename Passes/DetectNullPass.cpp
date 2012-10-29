/*
 * DetectNullPass.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: ice-phoenix
 */

#include "DetectNullPass.h"

#include <llvm/Constants.h>
#include <llvm/Support/InstVisitor.h>

namespace borealis {

using util::contains;
using util::containsKey;
using util::enums::asInteger;
using util::streams::endl;
using util::toString;

////////////////////////////////////////////////////////////////////////////////
//
// Regular instruction visitor
//
////////////////////////////////////////////////////////////////////////////////

class RegularDetectNullInstVisitor :
        public llvm::InstVisitor<RegularDetectNullInstVisitor> {

public:

    using llvm::InstVisitor<RegularDetectNullInstVisitor>::visit;

    RegularDetectNullInstVisitor(DetectNullPass::NullInfoMap& NIM) : NIM(NIM) {
    }

    void visit(llvm::Instruction& I) {
        using llvm::Value;

        const Value* ptr = &I;
        if (containsKey(NIM, ptr)) return;

        InstVisitor::visit(I);
    }

    void visitCallInst(llvm::CallInst& I) {
        if (!I.getType()->isPointerTy()) return;

        NIM[&I] = NullInfo().setStatus(NullStatus::Maybe_Null);
    }

    void visitStoreInst(llvm::StoreInst& I) {
        using namespace::llvm;

        const Value* ptr = I.getPointerOperand();
        const Value* value = I.getValueOperand();

        if (!ptr->getType()->getPointerElementType()->isPointerTy()) return;

        if (isa<ConstantPointerNull>(*value)) {
            NIM[ptr] = NullInfo().setType(NullType::DEREF).setStatus(NullStatus::Null);
        }
    }

    void visitInsertValueInst(llvm::InsertValueInst& I) {
        using namespace::llvm;

        const Value* value = I.getInsertedValueOperand();
        if (isa<ConstantPointerNull>(value)) {
            const std::vector<unsigned> idxs = I.getIndices().vec();
            NIM[&I] = NullInfo().setStatus(idxs, NullStatus::Null);
        }
    }

private:

    DetectNullPass::NullInfoMap& NIM;

};

////////////////////////////////////////////////////////////////////////////////
//
// PHINode instruction visitor
//
////////////////////////////////////////////////////////////////////////////////

class PHIDetectNullInstVisitor :
        public llvm::InstVisitor<PHIDetectNullInstVisitor> {

public:

    PHIDetectNullInstVisitor(DetectNullPass::NullInfoMap& NIM) : NIM(NIM) {
    }

    void visitPHINode(llvm::PHINode& I) {
        using namespace::llvm;

        if (!I.getType()->isPointerTy()) return;

        std::set<const PHINode*> visited;
        auto incomingValues = getIncomingValues(I, visited);

        NullInfo nullInfo = NullInfo();
        for (const Value* II : incomingValues) {
            if (containsKey(NIM, II)) {
                nullInfo = nullInfo.merge(NIM[II]);
            } else {
                nullInfo = nullInfo.merge(NullStatus::Not_Null);
            }
        }
        NIM[&I] = nullInfo;
    }

private:

    DetectNullPass::NullInfoMap& NIM;

    std::set<const llvm::Value*> getIncomingValues(
            const llvm::PHINode& I,
            std::set<const llvm::PHINode*>& visited) {
        using namespace::llvm;

        std::set<const Value*> res;

        if (contains(visited, &I)) return res;
        else visited.insert(&I);

        for (unsigned i = 0U; i < I.getNumIncomingValues(); ++i) {
            const Value* incoming = I.getIncomingValue(i);
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

DetectNullPass::DetectNullPass() : llvm::FunctionPass(ID) {
	// TODO
}

void DetectNullPass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
}

bool DetectNullPass::runOnFunction(llvm::Function& F) {

	init();

	RegularDetectNullInstVisitor regularVisitor(data);
	regularVisitor.visit(F);

	PHIDetectNullInstVisitor phiVisitor(data);
	phiVisitor.visit(F);

	return false;
}

DetectNullPass::~DetectNullPass() {
	// TODO
}

llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const NullInfo& info) {
	using namespace::std;

    s << asInteger(info.type) << ":" << endl;
	for(const auto& entry : info.offsetInfoMap) {
		s << entry.first << "->" << asInteger(entry.second) << endl;
	}

	return s;
}

} /* namespace borealis */

char borealis::DetectNullPass::ID;
static llvm::RegisterPass<borealis::DetectNullPass>
X("detect-null", "Explicit NULL assignment detector", false, false);
