//
// Created by abdullin on 10/31/17.
//

#include "ConditionSplitter.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ps {

ConditionSplitter::ConditionSplitter(Term::Ptr condition, Domenaizer domenize)
        : ObjectLevelLogging("ps-interpreter"), condition_(condition), domenize_(domenize) {}

ConditionSplitter::TermMap ConditionSplitter::apply() {
    if (auto&& cmp = llvm::dyn_cast<CmpTerm>(condition_.get())) {
        visitCmpTerm(cmp);
    } else if (auto&& bin = llvm::dyn_cast<BinaryTerm>(condition_.get())) {
        visitBinaryTerm(bin);
    } else if (llvm::isa<type::Bool>(condition_->getType().get())) {
        auto condDomain = domenize_(condition_);
        result_[condition_] = {condDomain, condDomain};
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
    auto lhvDomain = domenize_(lhv);
    auto rhvDomain = domenize_(rhv);
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

void ConditionSplitter::visitBinaryTerm(const BinaryTerm* term) {
    auto lhv = term->getLhv();
    auto rhv = term->getRhv();
    auto lhvDomain = domenize_(lhv);
    auto rhvDomain = domenize_(rhv);
    ASSERT(lhv && rhv, "cmp args of " + term->getName());

    auto&& andImpl = [] (Split lhv, Split rhv) -> Split {
        return {lhv.true_->meet(rhv.true_), lhv.false_->join(rhv.false_)};
    };
    auto&& orImpl = [] (Split lhv, Split rhv) -> Split {
        return {lhv.true_->join(rhv.true_), lhv.false_->join(rhv.false_)};
    };

    auto lhvValues = ConditionSplitter(lhv, domenize_).apply();
    auto rhvValues = ConditionSplitter(rhv, domenize_).apply();

    for (auto&& it : lhvValues) {
        auto value = it.first;
        auto lhvSplit = it.second;
        auto&& rhvit = util::at(rhvValues, value);
        if (not rhvit) continue;
        auto rhvSplit = rhvit.getUnsafe();

        Split temp1, temp2;
        switch (term->getOpcode()) {
            case llvm::ArithType::LAND:
            case llvm::ArithType::BAND:
                result_[value] = andImpl(lhvSplit, rhvSplit);
                break;
            case llvm::ArithType::LOR:
            case llvm::ArithType::BOR:
                result_[value] = orImpl(lhvSplit, rhvSplit);
                break;
            case llvm::ArithType::XOR:
                // XOR = (!lhv AND rhv) OR (lhv AND !rhv)
                temp1 = andImpl({lhvSplit.false_, lhvSplit.true_}, rhvSplit);
                temp2 = andImpl(lhvSplit, {rhvSplit.false_, rhvSplit.true_});
                result_[value] = orImpl(temp1, temp2);
                break;
            case llvm::ArithType::IMPLIES:
                // IMPL = (!lhv) OR (rhv)
                result_[value] = orImpl(lhvSplit.swap(), rhvSplit);
                break;
            default:
                UNREACHABLE("Unexpected binary term in splitter: " + term->getName());
        }
    }
}

}   // namespace ps
}   // namespace absint
}   // namespace borealis

#include "Util/unmacros.h"