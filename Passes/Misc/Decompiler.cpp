#include "Decompiler.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

#include <typeinfo>

#include "Util/streams.hpp"

using namespace llvm;

namespace borealis {
namespace decompiler{

void DecompilerPass::countBlocksInfo(Loop* L) {
    auto&& head = L->getHeader();
    unsigned depth = L->getLoopDepth();
    if(L->getExitingBlock() == nullptr || L->getLoopLatch() == nullptr) {
            //add in-loop blocks
            for(int i = 0; i < L->getBlocks().size(); ++i) {
                bbInfo.addBlock(*L->getBlocks()[i], BBConfiguration{false, false, BBPosition::NONE, head,false});
            }
            bbInfo.addBlock(*head, BBConfiguration{true, false, BBPosition::NONE, nullptr,false});
        return;
    }
    //add in-loop blocks
    for(int i = 0; i < L->getBlocks().size(); ++i) {
        bbInfo.addBlock(*L->getBlocks()[i], BBConfiguration{false, false, BBPosition::IN_LOOP, head,false});
    }
    if(L->getHeader() == L->getExitingBlock()) {
        bbInfo.addBlock(*L->getHeader(), {false, false, BBPosition::WHILE_BEGIN, head,false});
        bbInfo.addBlock(*L->getLoopLatch(), {false, false, BBPosition::WHILE_END, head,false});
    } else {
        bbInfo.addBlock(*L->getHeader(), {false, false, BBPosition::DO_WHILE_BEGIN, head,false});
        bbInfo.addBlock(*L->getExitingBlock(), {false, false, BBPosition::DO_WHILE_END, head,false});
    }
    //check subloops
    auto&& subLoops = L->getSubLoops();
    Loop::iterator j,f;
    for(j = subLoops.begin(), f = subLoops.end(); j != f; ++j)
        countBlocksInfo(*j);
}

bool DecompilerPass::runOnModule(Module& M) {
    infos()<<"\n";
    di.displayGlobals(M);

    for (auto&& func_iter = M.begin(), func_iter_end = M.end(); func_iter != func_iter_end; ++func_iter) {
        bbInfo.clear();
        phiInfo.clear();
        Function &F = *func_iter;
        di.displayFunction(F);
        if (!F.isDeclaration()) {
            //get loops info
            LoopInfo &LI = getAnalysis<LoopInfo>(F);
            bool isInCycle = false;
            bool isDoWhile = false;
            for(LoopInfo::iterator i = LI.begin(), e = LI.end(); i != e; ++i) {
                countBlocksInfo(*i);
            }
            for(auto&& it = F.begin(), end = F.end(); it != end; ++it) {
                if(!bbInfo.isContains(*it)) bbInfo.addBlock(*it, BBConfiguration{false, false, BBPosition::NONE, nullptr,false});
            }
            di.setBasicBlockInfo(bbInfo);
            //add phi entries
            for(auto&& bb = F.begin(), end = F.end(); bb != end; ++bb) {
                for(auto&& it = bb->begin(), it_end = bb->end(); it != it_end; ++it){
                    if(auto&& phi = dyn_cast<PHINode>(it)) {
                        for(auto i = 0U; i < phi->getNumIncomingValues(); ++i) {
                            phiInfo.addEntry(phi->getIncomingBlock(i), PhiEntry{phi, phi->getIncomingValue(i)});
                        }
                    }
                }
            }
            di.setPhiInfo(phiInfo);
            //display function
            infos()<<" {\n";
            for(auto&& i = F.begin(), end = F.end(); i != end; ++i) {
                di.displayBasicBlock(*i, bbInfo.getPosition(*i));
            }
            infos()<<"}\n";
        }
        else infos()<<";\n";
        infos()<<"\n";
    }
    //prints all structs at the end of module
    di.displayStructs();
    return false;
}

char DecompilerPass::ID = 0;

static RegisterPass<DecompilerPass>
X("decompiler", "Decompiler pass", false, false);

} /* namespace decompiler */
} /* namespace borealis */
