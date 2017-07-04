/*
 * StateSlicer.h
 *
 *  Created on: Mar 16, 2015
 *      Author: ice-phoenix
 */

#ifndef STATE_TRANSFORMER_STATESLICER_H_
#define STATE_TRANSFORMER_STATESLICER_H_

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/AliasSetTracker.h>

#include <unordered_set>

#include "State/PredicateState.def"
#include "State/Transformer/CachingTransformer.hpp"
#include "State/Transformer/ControlFlowDepsTracker.h"
#include "State/Transformer/LocalStensgaardAA.h"

namespace borealis {

class AliasAnalysisAdapter: public LocalAABase {
    llvm::AliasAnalysis* AA;
    std::unique_ptr<llvm::AliasSetTracker> AST;

    FactoryNest FN;

    llvm::Value* term2value(Term::Ptr t) {
        if (auto* vt = llvm::dyn_cast<ValueTerm>(t)) {
            if (vt->isGlobal()) {
                return const_cast<llvm::Value*>(FN.Slot->getGlobalValue(vt->getVName()));
            } else {
                return const_cast<llvm::Value*>(FN.Slot->getLocalValue(vt->getVName()));
            }
        } else if (auto* at = llvm::dyn_cast<ArgumentTerm>(t)) {
            return const_cast<llvm::Value*>(FN.Slot->getLocalValue(at->getName()));
        } else {
            // FIXME: akhin Logging
            return nullptr;
        }
    }

    uint64_t getLLVMAliasSize(llvm::Type* t) {
        if (auto* st = llvm::dyn_cast<llvm::StructType>(t)) {
            if (st->isOpaque()) {
                return llvm::AliasAnalysis::UnknownSize;
            }
        } else if (llvm::dyn_cast<llvm::FunctionType>(t)) {
            return llvm::AliasAnalysis::UnknownSize;
        }
        return AA->getTypeStoreSize(t);
    }

public:
    AliasAnalysisAdapter(llvm::AliasAnalysis* AA, FactoryNest FN):
        AA(AA), AST(new llvm::AliasSetTracker(*AA)), FN(FN){}

    bool mayAlias(Term::Ptr a, Term::Ptr b) override {
        if(*a == *b) return true;

        auto&& p = term2value(a);
        auto&& q = term2value(b);

#define AS_POINTER(V) \
    V, \
    getLLVMAliasSize(V->getType()->getPointerElementType()), \
    nullptr

        if (p and q) {
            AST->add(AS_POINTER(p));
            AST->add(AS_POINTER(q));
            return AST->getAliasSetForPointer(AS_POINTER(p)).aliasesPointer(AS_POINTER(q), *AA);
        } else {
            return true;
        }

#undef AS_POINTER
    }
};

class StateSlicer : public borealis::CachingTransformer<StateSlicer> {

    using Base = borealis::CachingTransformer<StateSlicer>;

public:

    StateSlicer(FactoryNest FN, PredicateState::Ptr query, llvm::AliasAnalysis* AA);
    StateSlicer(FactoryNest FN, PredicateState::Ptr query);

    using Base::transform;
    PredicateState::Ptr transform(PredicateState::Ptr ps);
    using Base::transformBase;
    Predicate::Ptr transformBase(Predicate::Ptr pred);
    PredicateState::Ptr transformBase(PredicateState::Ptr ps);
    PredicateState::Ptr transformChoice(PredicateStateChoicePtr ps);

    using TermSet = std::unordered_set<Term::Ptr, TermHash, TermEquals>;

private:

    PredicateState::Ptr query;

    Term::Set sliceVars;
    Term::Set slicePtrs;

    std::unique_ptr<LocalAABase> AA;
    ControlFlowDepsTracker CFDT;
    ControlFlowDepsTracker::PredicateSet currentPathDeps;

    void init(llvm::AliasAnalysis* llvmAA);
    void addSliceTerm(Term::Ptr term);

    bool checkPath(Predicate::Ptr pred, const TermSet& lhv, const TermSet& rhv);
    bool checkVars(const TermSet& lhv, const TermSet& rhv);
    bool checkPtrs(Predicate::Ptr pred, const TermSet& lhv, const TermSet& rhv);

    void addControlFlowDeps(Predicate::Ptr pred);

};

} /* namespace borealis */

#endif /* STATE_TRANSFORMER_STATESLICER_H_ */
