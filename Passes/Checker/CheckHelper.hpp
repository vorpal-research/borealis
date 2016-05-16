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
#include "State/Transformer/StateSlicer.h"
#include "State/Transformer/TermSizeCalculator.h"

namespace borealis {


template<class Pass>
class CheckHelper {

    Pass* pass;
    llvm::Instruction* I;
    DefectType defectType;

public:
    using explicit_result = smt::Result;

    CheckHelper(Pass* pass, llvm::Instruction* I, DefectType defectType) :
        pass(pass), I(I), defectType(defectType) {};

    bool skip() {
        return skip(pass->DM->getDefect(defectType, I));
    }
    bool skip(const DefectInfo& di) {
        if (pass->CM->shouldSkipInstruction(I)) return true;
        if (pass->DM->hasInfo(di)) return true;
        return false;
    }

    bool check(PredicateState::Ptr query, PredicateState::Ptr state) {
        return check(query, state, pass->DM->getDefect(defectType, I));
    }
    bool check(PredicateState::Ptr query, PredicateState::Ptr state, const DefectInfo& di) {
        TRACE_FUNC;

        static config::BoolConfigEntry logQueries("output", "smt-query-logging");
        bool noQueryLogging = not logQueries.get(false);

        dbgs() << "Query size:" << TermSizeCalculator::measure(query) << endl;
        dbgs() << "State size:" << TermSizeCalculator::measure(state) << endl;

        if (not query or not state) return false;
        if (query->isEmpty()) return false;
        if (state->isEmpty()) return true;

        dbgs() << "Defect: " << di << endl;
        dbgs() << "Checking: " << *I << endl;
        if(!noQueryLogging) dbgs() << "  Query: " << query << endl;

        dbgs() << "Slicing started" << endl;
        auto&& sliced = StateSlicer(pass->FN, query, pass->AA).transform(state);
        dbgs() << "Slicing finished" << endl;

        dbgs() << "State size after slicing:" << TermSizeCalculator::measure(sliced) << endl;

        if (state == sliced) {
            dbgs() << "Slicing failed" << endl;
        } else {
            if(!noQueryLogging) dbgs() << "Sliced: " << state << endl << "to: " << sliced << endl;
        }

        if(!noQueryLogging) dbgs() << "  State: " << sliced << endl;

        auto&& fMemInfo = pass->FM->getMemoryBounds(I->getParent()->getParent());

#if defined USE_MATHSAT_SOLVER
        MathSAT::ExprFactory ef;
        MathSAT::Solver s(ef, fMemInfo.first, fMemInfo.second);
#else
        //auto&& ef = util::threadLocalInstance<Z3::ExprFactory>();
        Z3::ExprFactory ef;
        Z3::Solver s(ef, fMemInfo.first, fMemInfo.second);
#endif

        auto solverResult = s.isViolated(query, sliced);
        if (auto satRes = solverResult.getSatPtr()) {
            pass->DM->addDefect(di);
            pass->DM->getAdditionalInfo(di).satModel = util::just(*satRes);
            pass->DM->getAdditionalInfo(di).atFunc = I->getParent()->getParent();
            pass->DM->getAdditionalInfo(di).atInst = I;
            dbgs() << "Defect confirmed: " << di << endl;
            return true;
        } else {
            pass->DM->addNoDefect(di);
            dbgs() << "Defect falsified: " << di << endl;
            return false;
        }
    }

    bool alias(llvm::Instruction* other) {
        auto&& otherDI = pass->DM->getDefect(defectType, other);
        auto&& di = pass->DM->getDefect(defectType, I);
        dbgs() << "Defect: " << di << endl;
        dbgs() << "Checking: " << *I << endl;
        dbgs() << "Using explicit defect result info" << endl;

        if(pass->DM->hasDefect(otherDI)) {
            pass->DM->addDefect(di);
            pass->DM->getAdditionalInfo(di).satModel = pass->DM->getAdditionalInfo(otherDI).satModel;
            pass->DM->getAdditionalInfo(di).atFunc = I->getParent()->getParent();
            pass->DM->getAdditionalInfo(di).atInst = I;
            dbgs() << "Defect confirmed as alias: " << di << endl;
            return true;
        } else {
            pass->DM->addNoDefect(di);
            dbgs() << "Defect falsified as alias: " << di << endl;
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

        auto&& fMemInfo = pass->FM->getMemoryBounds(I->getParent()->getParent());

        if (not state->isUnreachableIn(fMemInfo.first, fMemInfo.second)) {
            pass->DM->addDefect(di);
            return true;
        } else {
            pass->DM->addNoDefect(di);
            return false;
        }
    }

};

} // namespace borealis

#endif /* CHECKVISITOR_HPP_ */
