////
//// Created by abdullin on 11/22/17.
////
//
//#ifndef BOREALIS_ONEFORONEPSINTERPRETER_H
//#define BOREALIS_ONEFORONEPSINTERPRETER_H
//
//#include "Passes/Defect/DefectManager.h"
//#include "Passes/Tracker/SlotTrackerPass.h"
//#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
//
//namespace borealis {
//namespace absint {
//
//class OneForOneInterpreter: public logging::ObjectLevelLogging<OneForOneInterpreter> {
//public:
//
//    OneForOneInterpreter(const llvm::Instruction* I, SlotTrackerPass* ST, FactoryNest FN);
//
//    bool check(PredicateState::Ptr state, PredicateState::Ptr query, const DefectInfo& di);
//
//private:
//
//    const llvm::Instruction* I_;
//    SlotTrackerPass* ST_;
//    FactoryNest FN_;
//};
//
//}   // namespace absint
//}   // namespace borealis
//
//#endif //BOREALIS_ONEFORONEPSINTERPRETER_H
