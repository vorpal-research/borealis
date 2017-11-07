//
// Created by abdullin on 10/31/17.
//

#include "ConditionSplitter.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ps {

ConditionSplitter::ConditionSplitter(Term::Ptr condition, State::Ptr state)
        : condition_(condition), state_(state) {}

ConditionSplitter::TermMap ConditionSplitter::apply() {
    if (auto&& cmp = llvm::dyn_cast<CmpTerm>(condition_.get())) {
        visitCmpTerm(cmp);
    } else {
        errs() << "Unknown term in splitter: " << condition_ << endl;
    }
    return std::move(result_);
}

#define SPLIT_EQ(lhvDomain, rhvDomain) \
    result_[lhv] = lhvDomain->splitByEq(rhvDomain); \
    result_[rhv] = rhvDomain->splitByEq(lhvDomain);

#define SPLIT_NEQ(lhvDomain, rhvDomain) \
    result_[lhv] = lhvDomain->splitByEq(rhvDomain).swap(); \
    result_[rhv] = rhvDomain->splitByEq(lhvDomain).swap();

#define SPLIT_LESS(lhvDomain, rhvDomain) \
    result_[lhv] = lhvDomain->splitByLess(rhvDomain); \
    result_[rhv] = rhvDomain->splitByLess(lhvDomain).swap();

#define SPLIT_SLESS(lhvDomain, rhvDomain) \
    result_[lhv] = lhvDomain->splitBySLess(rhvDomain); \
    temp = rhvDomain->splitBySLess(lhvDomain); \
    result_[rhv] = {temp.false_, temp.true_};

#define SPLIT_GREATER(lhvDomain, rhvDomain) \
    result_[rhv] = rhvDomain->splitByLess(lhvDomain); \
    result_[lhv] = lhvDomain->splitByLess(rhvDomain).swap();

#define SPLIT_SGREATER(lhvDomain, rhvDomain) \
    result_[rhv] = rhvDomain->splitBySLess(lhvDomain); \
    temp = lhvDomain->splitBySLess(rhvDomain); \
    result_[lhv] = {temp.false_, temp.true_};

void ConditionSplitter::visitCmpTerm(const CmpTerm* term) {
    auto lhv = term->getLhv();
    auto rhv = term->getRhv();
    auto lhvDomain = state_->find(lhv);
    auto rhvDomain = state_->find(rhv);
    ASSERT(lhv && rhv, "cmp args of " + term->getName());

    bool isPtr = llvm::isa<type::Pointer>(lhv->getType().get()) || llvm::isa<type::Pointer>(rhv->getType().get());
    bool isFloat = llvm::isa<type::Float>(lhv->getType().get());
    Split temp;
    switch (term->getOpcode()) {
        case llvm::ConditionType::EQ: SPLIT_EQ(lhvDomain, rhvDomain); break;
        case llvm::ConditionType::NEQ: SPLIT_NEQ(lhvDomain, rhvDomain); break;
        case llvm::ConditionType::GT:
        case llvm::ConditionType::GE:
            if (isPtr) break;
            if (isFloat) {
                SPLIT_GREATER(lhvDomain, rhvDomain);
            } else {
                SPLIT_SGREATER(lhvDomain, rhvDomain);
            }
            break;
        case llvm::ConditionType::LT:
        case llvm::ConditionType::LE:
            if (isPtr) break;
            if (isFloat) {
                SPLIT_LESS(lhvDomain, rhvDomain);
            } else {
                SPLIT_SLESS(lhvDomain, rhvDomain);
            }
            break;
        case llvm::ConditionType::UGT:
        case llvm::ConditionType::UGE:
            if (isPtr) break;
            SPLIT_GREATER(lhvDomain, rhvDomain);
            break;
        case llvm::ConditionType::ULT:
        case llvm::ConditionType::ULE:
            if (isPtr) break;
            SPLIT_LESS(lhvDomain, rhvDomain);
            break;
        case llvm::ConditionType::TRUE:
        case llvm::ConditionType::FALSE:
            break;
        default:
            UNREACHABLE("Unknown cast: " + term->getName());
    }
}

}   // namespace ps
}   // namespace absint
}   // namespace borealis

#include "Util/unmacros.h"