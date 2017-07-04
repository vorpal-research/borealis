//
// Created by ice-phoenix on 5/27/15.
//

#include <llvm/Pass.h>

#include "Passes/Defect/DefectManager.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Reanimator/Reanimator.h"
#include "State/Transformer/AggregateTransformer.h"
#include "State/Transformer/ArrayBoundsCollector.h"
#include "State/Transformer/TermCollector.h"
#include "Util/passes.hpp"

#include "Util/macros.h"

namespace borealis {

class ReanimatorPass : public llvm::ModulePass,
                       public borealis::logging::ClassLevelLogging<ReanimatorPass> {

public:

    static char ID;

    ReanimatorPass() : llvm::ModulePass(ID) {};

    virtual bool runOnModule(llvm::Module&) override {
        auto&& DM = GetAnalysis<DefectManager>::doit(this);
        auto&& STP = GetAnalysis<SlotTrackerPass>::doit(this);

        for (auto&& di : DM.getData()) {
            auto&& adi = DM.getAdditionalInfo(di);

            if (not adi.satModel) continue;

            auto&& PSA = GetAnalysis<PredicateStateAnalysis>::doit(this, *adi.atFunc);
            auto&& ps = PSA.getInstructionState(adi.atInst);

            auto&& FN = FactoryNest(adi.atFunc->getDataLayout(), STP.getSlotTracker(adi.atFunc));

            auto&& TC = TermCollector<>(FN);
            auto&& AC = ArrayBoundsCollector(FN);
            (TC + AC).transform(ps);

            auto&& reanim = Reanimator(adi.satModel.getUnsafe(), AC.getArrayBounds());

            auto&& dbg = dbgs();

            dbg << ps << endl;
            dbg << adi.satModel << endl;
            dbg << reanim.getArrayBoundsMap() << endl;
            for (auto&& v : util::viewContainer(TC.getTerms())
                            .filter(APPLY(llvm::is_one_of<ArgumentTerm, ValueTerm>))) {
                dbg << raise(reanim, v);
            }
        }

        return false;
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        AU.setPreservesAll();

        AUX<DefectManager>::addRequiredTransitive(AU);
        AUX<SlotTrackerPass>::addRequiredTransitive(AU);

        AUX<PredicateStateAnalysis>::addRequiredTransitive(AU);
    }

    virtual ~ReanimatorPass() = default;

};

char ReanimatorPass::ID;
static RegisterPass<ReanimatorPass>
X("reanimator", "Pass that reanimates `variables` at defect sites");

} // namespace borealis

#include "Util/unmacros.h"
