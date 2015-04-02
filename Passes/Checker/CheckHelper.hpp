/*
 * CheckVisitor.hpp
 *
 *  Created on: Jan 31, 2014
 *      Author: ice-phoenix
 */

#ifndef CHECKVISITOR_HPP_
#define CHECKVISITOR_HPP_

#include "Logging/logger.hpp"
#include "Passes/Defect/DefectManager/DefectInfo.h"
#include "SMT/MathSAT/Solver.h"
#include "SMT/Z3/Solver.h"
#include <State/Transformer/StateSlicer.h>

namespace borealis {

template<class Pass>
class CheckHelper {

    Pass* pass;
    llvm::Instruction* I;
    DefectType defectType;

public:

    CheckHelper(Pass* pass, llvm::Instruction* I, DefectType defectType) :
        pass(pass), I(I), defectType(defectType) {};

    bool skip() {
        return skip(pass->DM->getDefect(defectType, I));
    }
    bool skip(const DefectInfo& di) {
        if (pass->CM->shouldSkipInstruction(I)) return true;
        if (pass->DM->hasDefect(di)) return true;
        return false;
    }

    bool check(PredicateState::Ptr query, PredicateState::Ptr state) {
        return check(query, state, pass->DM->getDefect(defectType, I));
    }
    bool check(PredicateState::Ptr query, PredicateState::Ptr state, const DefectInfo& di) {

        if (not query or not state) return false;

        auto&& sliced = StateSlicer(pass->FN, query, pass->AA).transform(state);
        if (*state == *sliced) {
            dbgs() << "Slicing failed" << endl;
        } else {
            dbgs() << "Sliced: " << state << endl << "to: " << sliced << endl;
        }

        dbgs() << "Checking: " << *I << endl;
        dbgs() << "  Query: " << query << endl;
        dbgs() << "  State: " << sliced << endl;

        auto fMemId = pass->FM->getMemoryStart(I->getParent()->getParent());

#if defined USE_MATHSAT_SOLVER
        MathSAT::ExprFactory ef;
        MathSAT::Solver s(ef, fMemId);
#else
        Z3::ExprFactory ef;
        Z3::Solver s(ef, fMemId);
#endif

        if (s.isViolated(query, sliced)) {
            pass->DM->addDefect(di);
            return true;
        } else {
            return false;
        }
    }

    bool isReachable(PredicateState::Ptr state) {
        return isReachable(state, pass->DM->getDefect(defectType, I));
    }
    bool isReachable(PredicateState::Ptr state, const DefectInfo& di) {

        if (not state) return false;

        dbgs() << "Checking: " << *I << endl;
        dbgs() << "  State: " << state << endl;

        auto fMemId = pass->FM->getMemoryStart(I->getParent()->getParent());

        if (not state->isUnreachableIn(fMemId)) {
            pass->DM->addDefect(di);
            return true;
        } else {
            return false;
        }
    }

};

} // namespace borealis

#endif /* CHECKVISITOR_HPP_ */
