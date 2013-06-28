/*
 * DetectNullPass.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Constants.h>
#include <llvm/Support/InstVisitor.h>

#include "Passes/DetectNullPass.h"

namespace borealis {

using borealis::util::contains;
using borealis::util::containsKey;

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

        if (containsKey(pass->data, &I)) return;

        InstVisitor::visit(I);
    }

    void visitCallInst(llvm::CallInst& I) {
        if (!I.getType()->isPointerTy()) return;

        pass->data[&I] = NullInfo().setStatus(NullStatus::MaybeNull);
    }

    void visitStoreInst(llvm::StoreInst& I) {
        using namespace llvm;

        Value* ptr = I.getPointerOperand();
        Value* value = I.getValueOperand();

        if (!ptr->getType()->getPointerElementType()->isPointerTy()) return;

        if (isa<ConstantPointerNull>(value)) {
            pass->data[ptr] = NullInfo().setType(NullType::DEREF).setStatus(NullStatus::Null);
        }
    }

    void visitInsertValueInst(llvm::InsertValueInst& I) {
        using namespace llvm;

        Value* value = I.getInsertedValueOperand();

        if (isa<ConstantPointerNull>(value)) {
            pass->data[&I] = NullInfo().setStatus(I.getIndices().vec(), NullStatus::Null);
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
        auto incomingValues = getIncomingValues(&I, visited);

        NullInfo nullInfo;
        for (auto* II : incomingValues) {
            if (containsKey(pass->data, II)) {
                nullInfo = nullInfo.merge(pass->data.at(II));
            } else {
                nullInfo = nullInfo.merge(NullStatus::NotNull);
            }
        }
        pass->data[&I] = nullInfo;
    }

private:

    DetectNullPass* pass;

    std::set<llvm::Value*> getIncomingValues(
            llvm::PHINode* I,
            std::set<llvm::PHINode*>& visited) {
        using namespace llvm;

        std::set<Value*> res;

        if (contains(visited, I)) return res;
        else visited.insert(I);

        for (unsigned i = 0U; i < I->getNumIncomingValues(); ++i) {
            Value* incoming = I->getIncomingValue(i);
            if (auto* iPhi = dyn_cast<PHINode>(incoming)) {
                auto sub = getIncomingValues(iPhi, visited);
                res.insert(sub.begin(), sub.end());
            } else {
                res.insert(incoming);
            }
        }

        return res;
    }
};

////////////////////////////////////////////////////////////////////////////////

DetectNullPass::DetectNullPass() : ProxyFunctionPass(ID) {}
DetectNullPass::DetectNullPass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void DetectNullPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
}

bool DetectNullPass::runOnFunction(llvm::Function& F) {
    init();

    //Add maybe-nulls for function arguments
    for (auto& arg : F.getArgumentList()) {
        if (arg.getType()->isPointerTy()) {
            data[&arg] = NullInfo().setStatus(NullStatus::MaybeNull);
        }
    }

    RegularDetectNullInstVisitor regularVisitor(this);
    regularVisitor.visit(F);

    PHIDetectNullInstVisitor phiVisitor(this);
    phiVisitor.visit(F);

    return false;
}

DetectNullPass::~DetectNullPass() {}

char DetectNullPass::ID;
static RegisterPass<DetectNullPass>
X("detect-null", "Explicit NULL assignment detector");

} /* namespace borealis */
