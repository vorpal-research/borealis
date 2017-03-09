#include <llvm/IR/CFG.h>
#include <llvm/Support/GraphWriter.h>

#include "Config/config.h"
#include "Logging/tracer.hpp"
#include "Passes/PredicateAnalysis/Defines.def"
#include "Passes/PredicateStateAnalysis/OneForAllTD.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/AggregateTransformer.h"
#include "State/Transformer/Cropper.h"
#include "State/Transformer/CallSiteInitializer.h"
#include "State/Transformer/MemoryContextSplitter.h"
#include "State/Transformer/TermSizeCalculator.h"
#include "State/Transformer/GraphBuilder.h"

#include "Util/macros.h"

namespace borealis {

OneForAllTD::OneForAllTD() :
    ProxyFunctionPass(ID),
    SO{FN}, RR{FN}, ME{FN},
    NULLPTRIFY3(DT, FM, SLT) {}

OneForAllTD::OneForAllTD(llvm::Pass* pass) :
    ProxyFunctionPass(ID, pass),
    SO{FN}, RR{FN}, ME{FN},
    NULLPTRIFY3(DT, FM, SLT) {}

void OneForAllTD::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<llvm::PostDominatorTree>::addRequired(AU);
    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);

#define HANDLE_ANALYSIS(CLASS) \
    AUX<CLASS>::addRequiredTransitive(AU);
#include "Passes/PredicateAnalysis/Defines.def"
#undef HANDLE_ANALYSIS

}

bool OneForAllTD::runOnFunction(llvm::Function &F) {
    init();

    DT = &GetAnalysis<llvm::PostDominatorTree>::doit(this, F);
    FM = &GetAnalysis<FunctionManager>::doit(this, F);
    SLT = &GetAnalysis<SourceLocationTracker>::doit(this, F);

    FN = FactoryNest(F.getDataLayout(), GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F));

    SO = StateOptimizer{FN};
    RR = Retyper{FN};
    ME = MarkEraser{FN};

#define HANDLE_ANALYSIS(CLASS) \
    PA.push_back(static_cast<AbstractPredicateAnalysis*>(&GetAnalysis<CLASS>::doit(this, F)));
#include "Passes/PredicateAnalysis/Defines.def"

    // Register globals in our predicate state
    auto&& gState = FN.getGlobalState(&F);

    dbgs() << "Global state size: " << TermSizeCalculator::measure(gState) << endl;
    // Register requires
    auto&& requires = FM->getReq(&F);
    // Memory split requires
    auto&& mcs = MemoryContextSplitter(FN);
    auto&& splittedRequires = mcs.transform(requires);

    auto&& initialStateBuilder = PredicateStateBuilder(FN.State);
    initialStateBuilder += gState;
    initialStateBuilder += mcs.getGeneratedPredicates();
    initialStateBuilder += splittedRequires;

    // Register arguments as visited values
    for (auto&& arg : F.getArgumentList()) {
        initialStateBuilder <<= SLT->getLocFor(&arg);
    }

    // Save initial state
    this->initialState = initialStateBuilder.apply();

    auto finalStateBuilder = initialStateBuilder;

    finalStateBuilder += buildFunctionBodyState(&F);
    this->finalState = SO.transform(finalStateBuilder.apply());

    dbgs() << "Initial state size: " << TermSizeCalculator::measure(initialState) << endl;
    dbgs() << "Final state size: " << TermSizeCalculator::measure(finalState) << endl;

    return false;
}

PredicateState::Ptr OneForAllTD::getInstructionState(const llvm::Instruction *I) {
    if(auto&& s = instructionStates[I]) {
        return s;
    } else {
        auto lastPred = backmapping[I];
        // exploiting the naughty scope weirdness that makes s visible in `else`
        s = cropStateOn(this->finalState, FN, LAM(pred, pred->equals(lastPred.get())));
        s = ME.transform(s);

        if (PredicateStateAnalysis::OptimizeStates()) {
            dbgs() << "Optimizer started" << endl;
            s = SO.transform(s);
            dbgs() << "Optimizer finished" << endl;
        }

        s = RR.transform(s);
        return s;
    }
}

void OneForAllTD::init() {
    AbstractPredicateStateAnalysis::init();
    PA.clear();
    basics.clear();
    backmapping.clear();
    betweens.clear();
}

void OneForAllTD::finalize() {
    AbstractPredicateStateAnalysis::finalize();
}

std::vector<Predicate::Ptr> OneForAllTD::predicatesFor(const llvm::Instruction *I) {
    using borealis::util::containsKey;

    std::vector<Predicate::Ptr> res;

    for (auto* APA : PA) {
        auto&& map = APA->getPredicateMap();
        if (containsKey(map, I)) {
            res.push_back(map.at(I));
        }
    }

    return std::move(res);
}


PredicateState::Ptr OneForAllTD::basicPSFor(const llvm::BasicBlock *BB) {
    if(auto&& existing = util::at(basics, BB)) return existing.getUnsafe();

    auto res = PredicateStateBuilder(FN.State);
    for(auto&& I : *BB) {
        auto instructionStateBuilder = PredicateStateBuilder(FN.State);

        auto iPreds = predicatesFor(&I);
        if(iPreds.empty()) // add a nop so that every instruction maps to at least one predicate
            iPreds.push_back(FN.Predicate->getMarkPredicate(FN.Term->getOpaqueConstantTerm(reinterpret_cast<int64_t>(&I))));
        backmapping[&I] = iPreds.back();

        for(auto&& pred : iPreds) {
            instructionStateBuilder += pred;
        }
        instructionStateBuilder <<= SLT->getLocFor(&I);

        if(auto&& call = llvm::dyn_cast<llvm::CallInst>(&I)) {
            auto&& callStateBuilder = PredicateStateBuilder(FN.State);
            callStateBuilder += FM->getBdy(*call, FN);
            callStateBuilder += FM->getEns(*call, FN);

            auto&& instantiatedCallState =
                CallSiteInitializer(call, FN).transform(callStateBuilder.apply());

            instructionStateBuilder += instantiatedCallState;
        }
        res += instructionStateBuilder.apply();
    }

    return basics[BB] = res.apply();
}

// state _between_ BB (inclusive) and Dom (non-inclusive), assuming Dom is either a dominator of BB or nullptr
PredicateState::Ptr OneForAllTD::stateBetween(const llvm::BasicBlock *BB, const llvm::BasicBlock* Dom) {
    auto&& s = betweens[std::make_pair(BB, Dom)];
    if(s) return s;

    ASSERTC(BB != nullptr);
    if(Dom == BB) return s = FN.State->Basic();

    auto res = PredicateStateBuilder(FN.State);
    res += basicPSFor(BB);

    const llvm::BasicBlock* idom = getIDom(BB);

    std::vector<PredicateState::Ptr> choices;
    for(const llvm::BasicBlock* succ: util::view(llvm::succ_begin(BB), llvm::succ_end(BB))) {
        auto succState = PredicateStateBuilder(FN.State);

        std::vector<Predicate::Ptr> phiBlock;
        std::vector<Predicate::Ptr> termBlock;

        auto termPr = TerminatorBranch(BB->getTerminator(), succ);
        for (auto* APA : PA) {
            auto&& map = APA->getTerminatorPredicateMap();
            if (borealis::util::containsKey(map, termPr)) {
                termBlock.push_back(map.at(termPr));
            }
        }

        for(auto&& I : *succ) if(auto* phi = llvm::dyn_cast<llvm::PHINode>(&I)) {
            auto br = PhiBranch(BB, phi);
            for (auto* APA : PA) {
                auto&& map = APA->getPhiPredicateMap();
                if (borealis::util::containsKey(map, br)) {
                    phiBlock.push_back(map.at(br));
                }
            }
        }

        succState += FN.State->Basic(termBlock);
        succState += FN.State->Basic(phiBlock);
        succState += stateBetween(succ, idom);

        choices.push_back(succState.apply());
    }

    switch(choices.size()) {
        case 0: break;
        case 1: res += choices.front(); break;
        default: res += FN.State->Choice(choices); break;
    }

    if(idom != Dom) {
        res  += stateBetween(idom, Dom);
    }

    return s = res.apply();
}

template<class G>
static void popupGraph(G* g, bool wait = false, llvm::StringRef name = "") {
    static int pos = 0;
    ++pos;
    std::string realFileName = llvm::WriteGraph<G*>(g, "debug." + util::toString(pos), false, name);
    if (realFileName.empty()) return;
    llvm::DisplayGraph(realFileName, wait, llvm::GraphProgram::DOT);
}

PredicateState::Ptr OneForAllTD::buildFunctionBodyState(const llvm::Function *F) {
    auto entry = &F->getEntryBlock();

    return stateBetween(entry, nullptr);
}

const llvm::BasicBlock *OneForAllTD::getIDom(const llvm::BasicBlock *BB) {
    if(auto* node = (*DT)[const_cast<llvm::BasicBlock*>(BB)]->getIDom()) {
        return node->getBlock();
    }
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

char OneForAllTD::ID;
static RegisterPass<OneForAllTD>
    X("one-for-all-topdown", "One-for-all predicate state analysis");


} /* namespace borealis */

#include "Util/unmacros.h"
