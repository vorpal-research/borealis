//
// Created by belyaev on 12/17/15.
//

#include "Passes/Checker/CallGraphSlicer.h"
#include "Util/cast.hpp"
#include "Util/passes.hpp"
#include "Config/config.h"

#include <unordered_set>
#include <queue>

#include "Util/macros.h"

namespace borealis {

static config::MultiConfigEntry roots{"analysis", "root-function"};

void visitGlobal(const llvm::Value* value, std::unordered_set<const llvm::Function*>& functions) {
    if (auto func = llvm::dyn_cast<llvm::Function>(value)) {
        functions.insert(func);
    } else if (auto constantExpr = llvm::dyn_cast<llvm::ConstantExpr>(value)) {
        for (auto&& op : constantExpr->operands()) {
            visitGlobal(op, functions);
        }
    } else if (auto constantArray = llvm::dyn_cast<llvm::ConstantArray>(value)) {
        for (auto i = 0U; i < constantArray->getNumOperands(); ++i) {
            visitGlobal(constantArray->getOperand(i), functions);
        }
    } else if (auto&& constantStruct = llvm::dyn_cast<llvm::ConstantStruct>(value)) {
        for (auto i = 0U; i < constantStruct->getNumOperands(); ++i) {
            visitGlobal(constantStruct->getOperand(i), functions);
        }
    }
}

bool CallGraphSlicer::runOnModule(llvm::Module& M) {
    if(roots.size() == 0) return false;

    std::unordered_set<const llvm::Function*> keep;
    std::queue<const llvm::Function*> que;

    for (auto&& global: M.globals()) {
        if (not global.hasInitializer()) continue;
        visitGlobal(global.getInitializer(), addressTakenFunctions);
    }

    for(auto&& name: roots) {
        if(auto&& F = M.getFunction(name)) que.push(F);
    }

    for(auto&& F : M) {
        if(llvm::hasAddressTaken(F)) que.push(&F);
    }

    while(!que.empty()) {
        auto F = que.front();
        que.pop();

        if(F->isDeclaration()) continue;
        if(keep.count(F)) continue;
        keep.insert(F);

        auto called =
            util::viewContainer(*F)
            .flatten()
            .filter(LAM(it, llvm::is_one_of<llvm::CallInst, llvm::InvokeInst>(it)))
            .map(LAM(i, llvm::ImmutableCallSite(&i).getCalledFunction()))
            .filter()
            .toHashSet();

        for(auto&& F: called) {
            que.push(F);
        }

        for (auto&& inst : util::viewContainer(*F)
                .flatten()) {
            std::deque<llvm::Value*> operands(inst.op_begin(), inst.op_end());
            while (not operands.empty()) {
                auto&& op = operands.front();
                if (auto&& func = llvm::dyn_cast<llvm::Function>(op)) {
                    addressTakenFunctions.insert(func);
                } else if (auto&& ce = llvm::dyn_cast<llvm::ConstantExpr>(op)) {
                    for (auto&& it : util::viewContainer(ce->operand_values())) operands.push_back(it);
                }
                operands.pop_front();
            }
        }
    }
    addressTakenFunctions =
            util::viewContainer(addressTakenFunctions)
            .filter(LAM(f, not f->isDeclaration()))
            .filter(LAM(f, llvm::hasAddressTaken(*f)))
            .toHashSet();

    slice = std::move(keep);

    return false;
}

void CallGraphSlicer::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    llvm::Pass::getAnalysisUsage(AU);
    AU.setPreservesAll();
}

CallGraphSlicer::CallGraphSlicer(): llvm::ModulePass(ID) {}
CallGraphSlicer::~CallGraphSlicer() {}

char CallGraphSlicer::ID;
static RegisterPass<CallGraphSlicer> X("callgraph-slicer", "Pass that masks out unused functions");

} /* namespace borealis */

#include "Util/unmacros.h"
