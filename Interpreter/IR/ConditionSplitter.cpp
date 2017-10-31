//
// Created by abdullin on 6/9/17.
//

#include "ConditionSplitter.h"
#include "Util/llvm_matchers.hpp"
#include "Util/sayonara.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ir {

ConditionSplitter::ConditionSplitter(llvm::Value* target, Interpreter* interpreter, State::Ptr state)
        : ObjectLevelLogging("interpreter"),
          target_(target),
          interpreter_(interpreter),
          state_(state) {}

#include <functional-hell/matchers_fancy_syntax.h>

ConditionSplitter::ValueMap ConditionSplitter::apply() {
    using namespace functional_hell::matchers::placeholders;

    SWITCH(target_) {
        NAMED_CASE(c, llvm::$ICmpInst(_1, _2, _3)) {
            handleICmp(c->_1, c->_2, c->_3);
        }
        NAMED_CASE(c, llvm::$FCmpInst(_1, _2, _3)) {
            handleFCmp(c->_1, c->_2, c->_3);
        }
        NAMED_CASE(c, llvm::$BinaryOperator(_1, _2, _3)) {
            handleBinary(c->_1, c->_2, c->_3);
        }
    }
    return std::move(values_);
}

#include <functional-hell/matchers_fancy_syntax_off.h>

#define SPLIT_EQ(lhvDomain, rhvDomain) \
    values_[lhv] = lhvDomain->splitByEq(rhvDomain); \
    values_[rhv] = rhvDomain->splitByEq(lhvDomain);

#define SPLIT_NEQ(lhvDomain, rhvDomain) \
    values_[lhv] = lhvDomain->splitByEq(rhvDomain).swap(); \
    values_[rhv] = rhvDomain->splitByEq(lhvDomain).swap();

#define SPLIT_LESS(lhvDomain, rhvDomain) \
    values_[lhv] = lhvDomain->splitByLess(rhvDomain); \
    values_[rhv] = rhvDomain->splitByLess(lhvDomain).swap();

#define SPLIT_GREATER(lhvDomain, rhvDomain) \
    values_[rhv] = rhvDomain->splitByLess(lhvDomain); \
    values_[lhv] = lhvDomain->splitByLess(rhvDomain).swap();

void ConditionSplitter::handleICmp(llvm::Value* lhv, llvm::Value* rhv, const llvm::ICmpInst::Predicate& predicate) {
    auto&& lhvDomain = interpreter_->getVariable(lhv);
    auto&& rhvDomain = interpreter_->getVariable(rhv);
    ASSERT(lhvDomain && rhvDomain, "Unknown values in icmp splitter");

    Split temp;
    switch (predicate) {
        case llvm::CmpInst::ICMP_EQ: SPLIT_EQ(lhvDomain, rhvDomain); break;

        case llvm::CmpInst::ICMP_NE: SPLIT_NEQ(lhvDomain, rhvDomain); break;

        case llvm::CmpInst::ICMP_ULT:
        case llvm::CmpInst::ICMP_ULE:
            if (lhvDomain->isPointer() && rhvDomain->isPointer())
                break;
            SPLIT_LESS(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::ICMP_UGT:
        case llvm::CmpInst::ICMP_UGE:
            if (lhvDomain->isPointer() && rhvDomain->isPointer())
                break;
            SPLIT_GREATER(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::ICMP_SLT:
        case llvm::CmpInst::ICMP_SLE:
            values_[lhv] = lhvDomain->splitBySLess(rhvDomain);
            temp = rhvDomain->splitBySLess(lhvDomain);
            values_[rhv] = {temp.false_, temp.true_};
            break;

        case llvm::CmpInst::ICMP_SGT:
        case llvm::CmpInst::ICMP_SGE:
            values_[rhv] = rhvDomain->splitBySLess(lhvDomain);
            temp = lhvDomain->splitBySLess(rhvDomain);
            values_[lhv] = {temp.false_, temp.true_};
            break;

        default:
            UNREACHABLE("Unknown operation in icmp");
    }
}

void ConditionSplitter::handleFCmp(llvm::Value* lhv, llvm::Value* rhv, const llvm::FCmpInst::Predicate& predicate) {
    auto&& lhvDomain = interpreter_->getVariable(lhv);
    auto&& rhvDomain = interpreter_->getVariable(rhv);
    ASSERT(lhvDomain && rhvDomain, "Unknown values in fcmp splitter");

    Split temp;
    switch (predicate) {
        case llvm::CmpInst::FCMP_OEQ:
        case llvm::CmpInst::FCMP_UEQ:
            SPLIT_EQ(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::FCMP_ONE:
        case llvm::CmpInst::FCMP_UNE:
            SPLIT_NEQ(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::FCMP_OGT:
        case llvm::CmpInst::FCMP_UGT:
        case llvm::CmpInst::FCMP_OGE:
        case llvm::CmpInst::FCMP_UGE:
            SPLIT_LESS(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::FCMP_OLT:
        case llvm::CmpInst::FCMP_ULT:
        case llvm::CmpInst::FCMP_OLE:
        case llvm::CmpInst::FCMP_ULE:
            SPLIT_GREATER(lhvDomain, rhvDomain);
            break;

        default:
            UNREACHABLE("Unknown operation in fcmp");
    }
}

void ConditionSplitter::handleBinary(llvm::Value* lhv, llvm::Value* rhv, const llvm::BinaryOperator::BinaryOps opcode) {
    auto&& lhvDomain = interpreter_->getVariable(lhv);
    auto&& rhvDomain = interpreter_->getVariable(rhv);
    ASSERT(lhvDomain && rhvDomain, "Unknown values in binary splitter");

    auto&& andImpl = [] (Split lhv, Split rhv) -> Split {
        return {lhv.true_->meet(rhv.true_), lhv.false_->join(rhv.false_)};
    };
    auto&& orImpl = [] (Split lhv, Split rhv) -> Split {
        return {lhv.true_->join(rhv.true_), lhv.false_->join(rhv.false_)};
    };

    auto lhvValues = ConditionSplitter(lhv, interpreter_, state_).apply();
    auto rhvValues = ConditionSplitter(rhv, interpreter_, state_).apply();

    Split temp1, temp2;
    for (auto&& it : lhvValues) {
        auto value = it.first;
        auto lhvSplit = it.second;
        auto&& rhvit = util::at(rhvValues, value);
        if (not rhvit) continue;
        auto rhvSplit = rhvit.getUnsafe();

        switch (opcode) {
            case llvm::Instruction::And:
                values_[value] = andImpl(lhvSplit, rhvSplit);
                break;
            case llvm::Instruction::Or:
                values_[value] = orImpl(lhvSplit, rhvSplit);
                break;
            case llvm::Instruction::Xor:
                // (!lhv AND rhv) OR (lhv AND !rhv)
                temp1 = andImpl({lhvSplit.false_, lhvSplit.true_}, rhvSplit);
                temp2 = andImpl(lhvSplit, {rhvSplit.false_, rhvSplit.true_});
                values_[value] = orImpl(temp1, temp2);
                break;
            default:
                UNREACHABLE("Unknown binary operator");
        }
    }
}

}   /* namespace ir */
}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
