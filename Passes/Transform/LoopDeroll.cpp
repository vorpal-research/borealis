/*
 * LoopDeroll.cpp
 *
 *  Created on: Dec 7, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Analysis/CodeMetrics.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/LoopIterator.h>
#include <llvm/Analysis/ScalarEvolutionExpander.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/TypeBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <cmath>
#include <vector>
#include <llvm/Support/GraphWriter.h>
#include <llvm/IR/Verifier.h>

#include "Codegen/scalarEvolutions.h"
#include "Codegen/intrinsics_manager.h"
#include "Config/config.h"
#include "Logging/logger.hpp"
#include "Passes/Transform/LoopDeroll.h"
#include "Statistics/statistics.h"
#include "Util/passes.hpp"
#include "Util/functional.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

static Statistic LoopsEncountered("loop-deroll",
    "totalLoops", "Total loops encountered by the loop deroll pass");
static Statistic LoopsBackstabbed("loop-deroll",
    "backstabbed", "Loops backstabbed by the loop deroll pass");
static Statistic LoopsFullyUnrolled("loop-deroll",
    "fullyUnrolled", "Loops fully unrolled by the loop deroll pass");

LoopDeroll::LoopDeroll() : llvm::LoopPass(ID) {}

static inline bool isLoopTruelyInfinite(llvm::Loop* L) {
    using namespace llvm;
    SmallVector<BasicBlock*, 4> ExitBlocks;
    L->getExitBlocks(ExitBlocks);
    return (ExitBlocks.size() == 0);
}

static inline void RemapInstruction(
        llvm::Instruction& I,
        llvm::ValueToValueMapTy& VMap) {

    using namespace llvm;

    for (unsigned op = 0, e = I.getNumOperands(); op != e; ++op) {
        Value* Op = I.getOperand(op);
        ValueToValueMapTy::iterator It = VMap.find(Op);
        if (It != VMap.end()) {
            I.setOperand(op, It->second);
        // Attempt to remap internal MDNode values to cloned ones
        // to preserve debug information when unrolling
        } else if (auto* MDN = dyn_cast_or_null<MDNode>(Op)) {

            std::vector<Value*> mdn_values;
            mdn_values.reserve(MDN->getNumOperands());
            bool hasClonedValues = false;

            for (unsigned mdn_op = 0, ee = MDN->getNumOperands(); mdn_op != ee; ++mdn_op) {
                Instruction* OldVal = dyn_cast_or_null<Instruction>(MDN->getOperand(mdn_op));
                ValueToValueMapTy::iterator Mdn_It = VMap.find(OldVal);
                if (OldVal && Mdn_It != VMap.end()) {
                    mdn_values.push_back(Mdn_It->second);
                    hasClonedValues = true;
                } else {
                    mdn_values.push_back(OldVal);
                }
            }

            if (hasClonedValues) {
                I.setOperand(op, MDNode::get(MDN->getContext(), mdn_values));
            }
        }
    }

    if (isa<PHINode>(I)) {
        PHINode& PN = cast<PHINode>(I);
        for (unsigned i = 0, e = PN.getNumIncomingValues(); i != e; ++i) {
            ValueToValueMapTy::iterator It = VMap.find(PN.getIncomingBlock(i));
            if (It != VMap.end()) {
                PN.setIncomingBlock(i, cast<BasicBlock>(It->second));
            }
        }
    }
}

static inline llvm::BasicBlock* CreateUnreachableBasicBlock(
        llvm::Function* F,
        llvm::Twine Name) {

    using namespace llvm;

    BasicBlock* BB = BasicBlock::Create(
            F->getContext(),
            Name + Twine(".unreachable"),
            F
    );

    static auto&& f = IntrinsicsManager::getInstance().createIntrinsic(
        function_type::INTRINSIC_CONSUME,
        "void",
        llvm::FunctionType::get(llvm::Type::getVoidTy(F->getContext()), false),
        F->getParent()
    );
    CallInst::Create(f, "", BB);

    new UnreachableInst(F->getContext(), BB);

    return BB;
}

static inline llvm::BasicBlock* normalizePhiNodes(llvm::BasicBlock* BB) {
    using namespace llvm;

    auto phis = util::viewContainer(*BB)
                .filter(isaer<PHINode>())
                .map(caster<PHINode>());

    auto* movePos = BB->getFirstNonPHIOrDbgOrLifetime();

    for (auto& PHI : phis) {
        PHI.moveBefore(movePos);
    }

    return BB;
}

static llvm::BasicBlock* EvolveBasicBlock(
    llvm::ScalarEvolution* SE,
    llvm::BasicBlock* BB,
    llvm::Loop* L,
    const llvm::SCEV* Iteration,
    llvm::ValueToValueMapTy& VMap,
    std::vector<llvm::Instruction*>& toRemove
) {

    using namespace llvm;

    auto* bb = CloneBasicBlock(BB, VMap, ".bor.last");
    VMap[BB] = bb; // FIXME: we should first map all the blocks and then all the values

    for (auto& I : *bb) RemapInstruction(I, VMap);

    llvm::SCEVExpander exp(*SE, "bor.expander");

    for (auto& I : *BB) {
        if ( ! SE->isSCEVable(I.getType())) continue;
        if (isa<CmpInst>(I)) continue; // XXX: scalar evo does not handle cmps only, or... ???

        auto* scev = SE->getSCEV(&I);

        if (SE->isLoopInvariant(scev, L)) continue;
        if (isa<SCEVCouldNotCompute>(scev)) continue;

        llvm::LoopToScevMapT mp;
        mp.insert({L, Iteration});

        auto* evo = llvm::apply(scev, mp, *SE);

        llvm::Instruction* insertAt;
        if (auto* foo = dyn_cast<Instruction>(VMap[&I])) {
            insertAt = foo;
        } else {
            insertAt = bb->getFirstNonPHIOrDbgOrLifetime();
        }

        if( ! insertAt->getMetadata("dbg")) {
            insertAt->setMetadata("dbg", I.getMetadata("dbg"));
        }
        if( insertAt->getDebugLoc().isUnknown()) {
            insertAt->setDebugLoc(I.getDebugLoc());
        }

        auto* val = exp.expandCodeFor(evo, I.getType(), insertAt);
        if(auto ival = dyn_cast<Instruction>(val)) {
            ival->setMetadata("dbg", I.getMetadata("dbg"));
            ival->setDebugLoc(I.getDebugLoc());
        }


        Value* newI = VMap.lookup(&I);

        if (
            newI
         && (
                (val != &I && val != newI)
             || isa<PHINode>(val) // this case will be processed later by a higher entity
            )
        ) {
            newI->replaceAllUsesWith(val);
        }

        if ( ! insertAt->hasNUsesOrMore(1)) toRemove.push_back(insertAt);
    }

    normalizePhiNodes(bb);
    approximateAllDebugLocs(bb);
    return bb;
}

static util::option<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> UnrollFromTheBack(
    llvm::Function* F,
    llvm::Loop* L,
    llvm::LoopInfo* LI,
    llvm::LoopBlocksDFS::RPOIterator BlockBegin,
    llvm::LoopBlocksDFS::RPOIterator BlockEnd,
    llvm::ScalarEvolution* SE
) {

    using namespace ::llvm;
    using namespace ::llvm::types;

    auto& ctx = F->getContext();

    bool isInfinite = isLoopTruelyInfinite(L);

    if(
        util::view(L->block_begin(), L->block_end())
       .map(ops::dereference)
       .flatten()
       .filter(llvm::isaer<llvm::PHINode>{})
       .map(llvm::caster<llvm::PHINode>{})
       .any_of([SE](llvm::PHINode& phi){ return !SE->isSCEVable(phi.getType()); })
    ) return util::nothing();

    if ( ! isInfinite && ! SE->hasLoopInvariantBackedgeTakenCount(L)) return util::nothing();

    BasicBlock* Header = L->getHeader();
    BasicBlock* Latch = L->getLoopLatch();

    const SCEV* backEdgeTaken = isInfinite ? nullptr : SE->getBackedgeTakenCount(L);
    llvm::Type* indexType = backEdgeTaken ? backEdgeTaken->getType() : TypeBuilder<i<32>, true>::get(ctx);

    auto* nondetIntr = IntrinsicsManager::getInstance().createIntrinsic(
        function_type::INTRINSIC_NONDET,
        "size_t",
        FunctionType::get(indexType, false),
        F->getParent());
    auto* assumeIntr = IntrinsicsManager::getInstance().createIntrinsic(
        function_type::BUILTIN_BOR_ASSUME,
        "",
        TypeBuilder<void(i<1>), true>::get(ctx),
        F->getParent());

    llvm::IRBuilder<> builder{ F->getEntryBlock().getFirstNonPHIOrDbgOrLifetime() };

    auto* nondetCall = builder.CreateCall(nondetIntr, "bor.loop.iteration." + Header->getName());
    for(auto& I: *Header){
        const auto& dl = I.getDebugLoc();
        if ( ! dl.isUnknown()) {
            nondetCall->setDebugLoc(dl);
            break;
        }
    }
    for(auto& I: *Header){
        auto* md = I.getMetadata("dbg");
        if ( md ) {
            nondetCall->setMetadata("dbg", md);
            break;
        }
    }

    auto* nondetSCEV = SE->getSCEV(nondetCall);

    std::vector<BasicBlock*> NewBBs;
    ValueToValueMapTy VMap;

    BasicBlock* NewHeader = nullptr;
    BasicBlock* NewLatch = nullptr;

    std::vector<Instruction*> toRemove;

    for (auto BB = BlockBegin; BB != BlockEnd; ++BB) {
        BasicBlock* New = EvolveBasicBlock(SE, *BB, L, nondetSCEV, VMap, toRemove);
        {
            auto* insertAt = New->getFirstNonPHIOrDbgOrLifetime();
            IRBuilder<> builder{ insertAt };
            SCEVExpander exp(*SE, "bor.loop.expand");

            builder.CreateCall(
                assumeIntr,
                builder.CreateICmpSGE(nondetCall, ConstantInt::get(indexType, 0)),
                ""
            );

            if(!isInfinite) {
                auto* generatedBackEdge =
                    exp.expandCodeFor(backEdgeTaken, indexType, insertAt);

                // remember that backEdgeTaken is not the loop limit, but loop limit minus 1!
                builder.CreateCall(
                    assumeIntr,
                    builder.CreateICmpSLE(nondetCall, generatedBackEdge), // hence SLE
                    ""
                );
            }

        }
        F->getBasicBlockList().push_back(New);
        L->addBasicBlockToLoop(New, LI->getBase());
        NewBBs.push_back(New);

        // Add PHI entries for newly created BB to all exit blocks
        for (auto SI = succ_begin(*BB), SE = succ_end(*BB); SI != SE; ++SI) {
            if (L->contains(*SI))
                continue;
            for (auto BBI = (*SI)->begin(); PHINode* PHI = dyn_cast<PHINode>(BBI); ++BBI) {
                Value* Incoming = PHI->getIncomingValueForBlock(*BB);
                ValueToValueMapTy::iterator It = VMap.find(Incoming);
                if (It != VMap.end())
                    Incoming = It->second;
                PHI->addIncoming(Incoming, New);
            }
        }

        if (Header == *BB) NewHeader = New;
        if (Latch == *BB) NewLatch = New;
    }

    for (auto& I : util::viewContainer(NewBBs)
                         .map(ops::dereference)
                         .flatten()) RemapInstruction(I, VMap);
    for (auto* I : toRemove) if ( ! I->hasNUsesOrMore(1)) I->eraseFromParent();

    ASSERTC(NewHeader && NewLatch);

    return util::just(std::make_pair(NewHeader, NewLatch));
}

static unsigned adjustUnrollFactor(unsigned num, llvm::Loop* l) {
    auto limit2one = [](unsigned i){ return std::max(i, 1U); };
    unsigned loopDepth = l->getLoopDepth() + l->getSubLoops().size();
    constexpr unsigned basicBlockStd = 4U;

    loopDepth = limit2one(loopDepth);

    unsigned basicBlocks = l->getBlocks().size();

    num /= limit2one(basicBlocks/basicBlockStd);
    num = static_cast<unsigned>(std::lround(std::pow(num, 1. / loopDepth)));

    return limit2one(num);
}

void LoopDeroll::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    using namespace llvm;

    AUX<LoopInfo>::addRequiredTransitive(AU);
    AUX<LoopManager>::addRequiredTransitive(AU);
    AUX<ScalarEvolution>::addRequiredTransitive(AU);
    // Does NOT preserve CFG
}

bool LoopDeroll::runOnLoop(llvm::Loop* L, llvm::LPPassManager& LPM) {
    using namespace llvm;

    // Basic info about the loop
    BasicBlock* Header = L->getHeader();
    BasicBlock* Latch = L->getLoopLatch();
    BasicBlock* LoopPredecessor = L->getLoopPredecessor();

    // Sanity checks
    if (Header == nullptr) {
        BYE_BYE(bool, "Cannot unroll a loop with multiple headers");
    }
    if (Latch == nullptr) {
        BYE_BYE(bool, "Cannot unroll a loop with multiple latches");
    }
    if (LoopPredecessor == nullptr) {
        BYE_BYE(bool, "Cannot unroll a loop with multiple predecessors");
    }

    Function* F = Header->getParent();

    LI = &getAnalysis<LoopInfo>();
    LM = &getAnalysis<LoopManager>();
    SE = &getAnalysis<ScalarEvolution>();

    // Loop exit info
    BranchInst* BI = cast<BranchInst>(Latch->getTerminator());
    bool ContinueOnTrue = L->contains(BI->getSuccessor(0));

    ++LoopsEncountered;

    BasicBlock* LoopExit;
    if (BI->getNumSuccessors() == 1) {
        LoopExit = nullptr;
    } else {
        LoopExit = BI->getSuccessor(ContinueOnTrue);
    }

    // Cloning bookkeeping
    std::vector<BasicBlock*> Headers;
    std::vector<BasicBlock*> Latches;
    Headers.push_back(Header);
    Latches.push_back(Latch);

    // Value map for the last cloned iteration
    ValueToValueMapTy LastValueMap;

    // Original loop PHI nodes needed for the first iteration
    std::vector<PHINode*> OrigPHINodes;
    for (auto& I : *Header) {
        if (isa<PHINode>(I)) {
            OrigPHINodes.push_back(&cast<PHINode>(I));
        } else {
            break;
        }
    }

    // LLVM on-the-fly SSA update requires blocks to be processed in
    // reverse post-order, so that LastValueMap contains the correct value
    // at each exit
    LoopBlocksDFS DFS(L);
    DFS.perform(LI);
    // Stash the DFS iterators before adding blocks to the loop
    LoopBlocksDFS::RPOIterator BlockBegin = DFS.beginRPO();
    LoopBlocksDFS::RPOIterator BlockEnd = DFS.endRPO();

    static config::BoolConfigEntry PerformBackStab("analysis", "deroll-backstab");
    bool DoBackStab = PerformBackStab.get(false); // XXX: move to true when stable
    // Try to stab this loop in the back
    // Do this before everything else 'cause derolling may break scalar evolution
    util::option<std::pair<BasicBlock*, BasicBlock*>> lastHeaderAndLatch;
    if (DoBackStab) lastHeaderAndLatch = UnrollFromTheBack(F, L, LI, BlockBegin, BlockEnd, SE);

    static config::ConfigEntry<int> DerollCountOpt("analysis", "deroll-count");
    static config::ConfigEntry<int> MaxDerollCountOpt("analysis", "max-deroll-count");
    unsigned CurrentDerollCount = DerollCountOpt.get(3);
    unsigned Max = MaxDerollCountOpt.get(std::numeric_limits<int>::max());

    static config::BoolConfigEntry EnableAdaptiveDeroll("analysis", "adaptive-deroll");
    bool AdaptiveDerollEnabled = EnableAdaptiveDeroll.get(false); // XXX: move to true when stable

    // Try to guess the deroll count
    unsigned TripCount = SE->getSmallConstantTripCount(L, Latch);
    if (TripCount != 0) {
        ++LoopsFullyUnrolled;
        CurrentDerollCount = TripCount;
    }

    // Try to find unroll annotation for this loop
    unsigned AnnoUnrollCount = LM->getUnrollCount(L);
    if (AnnoUnrollCount != 0) CurrentDerollCount = AnnoUnrollCount;

    if(CurrentDerollCount > Max) {
        CurrentDerollCount = Max;
    }

    if(AdaptiveDerollEnabled && lastHeaderAndLatch) {
        ++LoopsBackstabbed;
        CurrentDerollCount = std::min(CurrentDerollCount, 1U);
    } else if(AdaptiveDerollEnabled) {
        CurrentDerollCount = adjustUnrollFactor(CurrentDerollCount, L);
    }

    // llvm::WriteGraph<const llvm::Function*>(F, "cfg.before." + valueSummary(F) + valueSummary(Header), true);

    for (unsigned UnrollIter = 0; UnrollIter != CurrentDerollCount; UnrollIter++) {
        std::vector<BasicBlock*> NewBlocks;

        for (auto BB = BlockBegin; BB != BlockEnd; ++BB) {
            ValueToValueMapTy VMap;
            BasicBlock* New = CloneBasicBlock(*BB, VMap, ".bor." + Twine(UnrollIter));
            F->getBasicBlockList().push_back(New);
            L->addBasicBlockToLoop(New, LI->getBase());

            // Loop over all of the PHI nodes in the block, changing them to use
            // the incoming values from the previous block
            if (Header == *BB) {
                for (auto* OrigPHINode : OrigPHINodes) {
                    PHINode* NewPHI = cast<PHINode>(VMap[OrigPHINode]);
                    Value* InVal = NewPHI->getIncomingValueForBlock(Latch);
                    if (Instruction* InValI = dyn_cast<Instruction>(InVal)) {
                        if (UnrollIter > 0 && L->contains(InValI)) {
                            InVal = LastValueMap[InValI];
                        }
                    }
                    VMap[OrigPHINode] = InVal;
                    New->getInstList().erase(NewPHI);
                }
            }

            // Update the running map of newest clones
            LastValueMap[*BB] = New;
            for (const auto& V : VMap) {
                LastValueMap[V.first] = V.second;
            }

            // Add PHI entries for newly created values to all exit blocks
            for (auto SI = succ_begin(*BB), SE = succ_end(*BB); SI != SE; ++SI) {
                if (L->contains(*SI))
                    continue;
                for (auto BBI = (*SI)->begin(); PHINode* PHI = dyn_cast<PHINode>(BBI); ++BBI) {
                    Value* Incoming = PHI->getIncomingValueForBlock(*BB);
                    ValueToValueMapTy::iterator It = LastValueMap.find(Incoming);
                    if (It != LastValueMap.end())
                        Incoming = It->second;
                    PHI->addIncoming(Incoming, New);
                }
            }

            if (Header == *BB) Headers.push_back(New);
            if (Latch == *BB) Latches.push_back(New);
            NewBlocks.push_back(New);
        }

        // Remap all instructions in the most recent iteration
        for (auto* BB : NewBlocks) {
            for (auto& I : *BB) {
                RemapInstruction(I, LastValueMap);
            }
        }
    }

    // Loop over the PHI nodes in the original block, setting incoming values
    for (auto* PN : OrigPHINodes) {
        PN->replaceAllUsesWith(PN->getIncomingValueForBlock(LoopPredecessor));
        Header->getInstList().erase(PN);
    }

    for (auto& hal : lastHeaderAndLatch) {
        Headers.push_back(hal.first);
        Latches.push_back(hal.second);
    }

    // Now that all the basic blocks for the unrolled iterations are in place,
    // set up the branches to connect them
    for (unsigned LL = 0, E = Latches.size(); LL != E; ++LL) {
        // The original branch was replicated in each unrolled iteration
        BranchInst* Term = cast<BranchInst>(Latches[LL]->getTerminator());

        // The branch destination
        unsigned DD = (LL + 1) % E;
        BasicBlock* Dest = Headers[DD];

        if (DD != 0) {
            Term->setSuccessor(!ContinueOnTrue, Dest);
        } else if (LoopExit) {
            Term->setSuccessor(ContinueOnTrue, LoopExit);
            BasicBlock* UBB = CreateUnreachableBasicBlock(F, Twine(Latches[LL]->getName()));
            Term->setSuccessor(!ContinueOnTrue, UBB);

            L->addBasicBlockToLoop(UBB, LI->getBase());

        } else {
            new UnreachableInst(Term->getContext(), Term);
            Term->eraseFromParent();
        }
    }

    // llvm::WriteGraph<const llvm::Function*>(F, "cfg.after." + valueSummary(F) + valueSummary(Header), true);

    // Reconstruct dom info, because it is not preserved properly
    if (DominatorTreeWrapperPass* DT = LPM.getAnalysisIfAvailable<DominatorTreeWrapperPass>()) {
        DT->runOnFunction(*F);
    }
    // This loop shall be no more...
    LPM.deleteLoopFromQueue(L);

    return true;
}

char LoopDeroll::ID;
static RegisterPass<LoopDeroll>
X("loop-deroll", "Loop de-roller pass");

} /* namespace borealis */

#include "Util/unmacros.h"
