/*
 * CheckVisitor.hpp
 *
 *  Created on: Jan 31, 2014
 *      Author: ice-phoenix
 */

#ifndef CHECKVISITOR_HPP_
#define CHECKVISITOR_HPP_


#include "Interpreter/PSInterpreterManager.h"
#include "Logging/logger.hpp"
#include "Passes/Defect/DefectManager/DefectInfo.h"
#include "SMT/MathSAT/Solver.h"
#include "SMT/Z3/Solver.h"
#include "SMT/Boolector/Solver.h"
#include "SMT/STP/Solver.h"
#include "SMT/CVC4/Solver.h"
#include "SMT/Portfolio/Solver.h"
#include "State/Transformer/GraphBuilder.h"
#include "State/Transformer/MemorySpacer.h"
#include "State/Transformer/PoorMem2Reg.h"
#include "State/Transformer/StateSlicer.h"
#include "State/Transformer/TermSizeCalculator.h"
#include "State/Transformer/Normalizer.h"
#include "Util/time.hpp"

#include "Util/macros.h"

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

        auto function = I->getParent()->getParent();
        auto PSM = absint::PSInterpreterManager(function, pass->DM, pass->ST,
                                                LAM(I, pass->getInstructionState(I)));
        return PSM.hasInfo(di);
    }

private:
    static smt::Result checkViolationZ3(
        std::pair<size_t, size_t> memoryBounds,
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
        Z3::ExprFactory ef;
        Z3::Solver s(ef, memoryBounds.first, memoryBounds.second);
        return s.isViolated(query, state);
    }
    static smt::Result checkViolationBoolector(
        std::pair<size_t, size_t> memoryBounds,
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
        Boolector::ExprFactory ef;
        Boolector::Solver s(ef, memoryBounds.first, memoryBounds.second);
        return s.isViolated(query, state);
    }
    static smt::Result checkViolationSTP(
        std::pair<size_t, size_t> memoryBounds,
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
        STP::ExprFactory ef;
        STP::Solver s(ef, memoryBounds.first, memoryBounds.second);
        return s.isViolated(query, state);
    }
    static smt::Result checkViolationCVC4(
        std::pair<size_t, size_t> memoryBounds,
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
        CVC4::ExprFactory ef;
        CVC4::Solver s(ef, memoryBounds.first, memoryBounds.second);
        return s.isViolated(query, state);
    }
    static smt::Result checkViolationMathSAT(
        std::pair<size_t, size_t> memoryBounds,
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
        MathSAT::ExprFactory ef;
        MathSAT::Solver s(ef, memoryBounds.first, memoryBounds.second);
        return s.isViolated(query, state);
    }
    static smt::Result checkViolationPortfolio(
        std::pair<size_t, size_t> memoryBounds,
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
        portfolio_::Solver s(memoryBounds.first, memoryBounds.second);
        return s.isViolated(query, state);
    }
    static smt::Result checkViolation(
        std::pair<size_t, size_t> memoryBounds,
        PredicateState::Ptr query,
        PredicateState::Ptr state) {
        static config::StringConfigEntry engine{ "analysis", "smt-engine" };
        auto engineName = engine.get("z3");

        borealis::util::StopWatch timer;
        using namespace std::chrono_literals;

//        ON_SCOPE_EXIT(
//            if(timer.duration() > 2000ms && state->size() < 3000) {
//                borealis::displayAsGraph(state, "Problem state");
//            }
//        );

        if(engineName == "mathsat") return checkViolationMathSAT(memoryBounds, query, state);
        if(engineName == "z3") return checkViolationZ3(memoryBounds, query, state);
        if(engineName == "cvc4") return checkViolationCVC4(memoryBounds, query, state);
        if(engineName == "boolector") return checkViolationBoolector(memoryBounds, query, state);
        if(engineName == "stp") return checkViolationSTP(memoryBounds, query, state);
        if(engineName == "portfolio") return checkViolationPortfolio(memoryBounds, query, state);
        // Pseudo-solvers useful for perf debugging
        if(engineName == "null") return smt::UnknownResult();
        if(engineName == "allsat") return smt::SatResult();
        if(engineName == "allunsat") return smt::UnsatResult();
        UNREACHABLE(tfm::format("Unknown solver specified: %s", engineName));
    }
public:

    bool check(PredicateState::Ptr query, PredicateState::Ptr state) {
        return check(query, state, pass->DM->getDefect(defectType, I));
    }
    bool check(PredicateState::Ptr query, PredicateState::Ptr state, const DefectInfo& di) {
        TRACE_FUNC;

        auto&& FN = pass->FN;
        auto&& ST = pass->ST;

        static config::BoolConfigEntry logQueries("output", "smt-query-logging");
        bool noQueryLogging = not logQueries.get(false);

        dbgs() << "Query size:" << TermSizeCalculator::measure(query) << endl;
        dbgs() << "State size:" << TermSizeCalculator::measure(state) << endl;

        dbgs() << "Defect: " << di << endl;
        dbgs() << "Checking: " << ST->toString(I) << endl;
        if(!noQueryLogging) dbgs() << "  Query: " << query << endl;

        if (not query or not state) return false;
        if (query->isEmpty()) return false;
        if (state->isEmpty()) return true;

        static config::BoolConfigEntry useLocalAA("analysis", "use-local-aa");
        static config::BoolConfigEntry doSlicing("analysis", "do-slicing");

        Normalizer nl(FN);
        state = nl.transform(state);
        query = nl.transform(query);

        MemorySpacer msp(FN, FN.State->Chain(state, query));
        state = msp.transform(state);
        query = msp.transform(query);

        PoorMem2Reg m2r(FN);
        state = m2r.transform(state);
        query = m2r.transform(query);

        if(doSlicing.get(true)) {
            dbgs() << "Slicing started" << endl;
            auto sliced = StateSlicer(FN, query, useLocalAA.get(false)? nullptr : pass->AA).transform(state);
            dbgs() << "Slicing finished" << endl;
            dbgs() << "State size after slicing:" << TermSizeCalculator::measure(sliced) << endl;
            if (state == sliced) {
                dbgs() << "Slicing failed" << endl;
            } else {
                if(!noQueryLogging) dbgs() << "Sliced: " << state << endl << "to: " << sliced << endl;
            }
            state = sliced;
        } else {
            dbgs() << "Slicing disabled" << endl;
        }

        static config::BoolConfigEntry memSpacing("analysis", "memory-spaces");
        if(memSpacing.get(false)) {
            dbgs() << "Memspacing started" << endl;
            MemorySpacer msp(FN, FN.State->Chain(state, query));
            state = msp.transform(state);
            query = msp.transform(query);
            dbgs() << "Memspacing finished" << endl;
        } else {
            dbgs() << "Memspacing disabled" << endl;
        }

        if(!noQueryLogging) dbgs() << "  State: " << state << endl;

        auto&& fMemInfo = pass->FM->getMemoryBounds(I->getParent()->getParent());

        auto solverResult = checkViolation(fMemInfo, query, state);
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
            if(solverResult.isUnknown()) dbgs() << "{Unknown}" << endl;
            else dbgs() << "{Unsat}" << endl;

            //if(solverResult.isUnknown()) {
            //    auto graph = buildGraphRep(state);
            //    llvm::ViewGraph(&graph, "Unknown state");
            //}

            return false;
        }
    }

    bool alias(llvm::Instruction* other) {
        auto&& ST = pass->ST;

        auto&& otherDI = pass->DM->getDefect(defectType, other);
        auto&& di = pass->DM->getDefect(defectType, I);
        dbgs() << "Defect: " << di << endl;
        dbgs() << "Checking: " << ST->toString(I) << endl;
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
        auto&& ST = pass->ST;

        dbgs() << "Checking: " << ST->toString(I) << endl;
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

#include "Util/unmacros.h"

#endif /* CHECKVISITOR_HPP_ */
