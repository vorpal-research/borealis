//
// Created by abdullin on 10/27/17.
//

#ifndef BOREALIS_PSINTERPRETERMANAGER_H
#define BOREALIS_PSINTERPRETERMANAGER_H

#include "Passes/Defect/DefectManager.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"

namespace borealis {
namespace absint {

class PSInterpreter: public logging::ObjectLevelLogging<PSInterpreter> {
public:

    using Statifier = std::function<PredicateState::Ptr(llvm::Instruction*)>;

    PSInterpreter(llvm::Function* F, DefectManager* DM, SlotTrackerPass* ST, Statifier statify);

    void interpret();
    bool hasInfo(const DefectInfo& info);

private:

    static std::unordered_set<llvm::Function*> interpreted_;

    llvm::Function* F_;
    DefectManager* DM_;
    SlotTrackerPass* ST_;
    Statifier statify_;
    FactoryNest FN_;
};

}   // namespace absint
}   // namespace borealis


#endif //BOREALIS_PSINTERPRETERMANAGER_H
