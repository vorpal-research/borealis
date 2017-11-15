//
// Created by abdullin on 11/15/17.
//

#ifndef BOREALIS_CONTRACTCHECKER_H
#define BOREALIS_CONTRACTCHECKER_H

#include "Interpreter.h"
#include "Passes/Defect/DefectManager.h"
#include "Passes/Manager/FunctionManager.h"
#include "Transformer.hpp"

namespace borealis {
namespace absint {
namespace ps {

class ContractChecker : public Transformer<ContractChecker>,
                        public logging::ObjectLevelLogging<ContractChecker> {
public:
    using Base = Transformer<ContractChecker>;

    ContractChecker(FactoryNest FN, llvm::Function* F,
                    DefectManager* DM, FunctionManager* FM,
                    DomainFactory* DF, Interpreter* interpreter);

    void apply();

    PredicateState::Ptr transformBasic(BasicPredicateStatePtr basic);
    Predicate::Ptr transformCallPredicate(CallPredicatePtr pred);

    void checkEns();

private:
    PredicateState::Ptr currentBasic_;
    llvm::Function* F_;
    DefectManager* DM_;
    FunctionManager* FM_;
    DomainFactory* DF_;
    Interpreter* interpreter_;
    std::unordered_map<DefectInfo, bool> defects_;
};

} // namespace ps
} // namespace absint
} // namespace borealis


#endif //BOREALIS_CONTRACTCHECKER_H
