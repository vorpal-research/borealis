/*
 * AnnotationCast.cpp
 *
 *  Created on: Jan 22, 2013
 *      Author: belyaev
 */

#include "Anno/anno.h"
#include "Term/Term.h"
#include "Term/TermFactory.h"

#include "Annotation/AnnotationCast.h"

#include "Annotation/Annotation.def"

#include "Util/macros.h"

using namespace borealis;

static bool isCompare(bin_opcode op) {
    switch(op) {
    case bin_opcode::OPCODE_EQ:
    case bin_opcode::OPCODE_NE:
    case bin_opcode::OPCODE_GT:
    case bin_opcode::OPCODE_LT:
    case bin_opcode::OPCODE_GE:
    case bin_opcode::OPCODE_LE:
        return true;
    default:
        return false;
    }
}

static llvm::ConditionType convertCT(bin_opcode op) {
    switch(op) {
    case bin_opcode::OPCODE_EQ:
        return llvm::ConditionType::EQ;
    case bin_opcode::OPCODE_NE:
        return llvm::ConditionType::NEQ;
    case bin_opcode::OPCODE_GT:
        return llvm::ConditionType::GT;
    case bin_opcode::OPCODE_LT:
        return llvm::ConditionType::LT;
    case bin_opcode::OPCODE_GE:
        return llvm::ConditionType::GTE;
    case bin_opcode::OPCODE_LE:
        return llvm::ConditionType::LTE;
    default:
        BYE_BYE(llvm::ConditionType, "Called with non-conditional operation");
    }
}

static llvm::ArithType convertAT(bin_opcode op) {
    switch(op) {
    case bin_opcode::OPCODE_PLUS:
        return llvm::ArithType::ADD;
    case bin_opcode::OPCODE_MINUS:
        return llvm::ArithType::SUB;
    case bin_opcode::OPCODE_MULT:
        return llvm::ArithType::MUL;
    case bin_opcode::OPCODE_DIV:
        return llvm::ArithType::DIV;
    case bin_opcode::OPCODE_MOD:
        return llvm::ArithType::REM;
    case bin_opcode::OPCODE_BAND:
        return llvm::ArithType::BAND;
    case bin_opcode::OPCODE_BOR:
        return llvm::ArithType::BOR;
    case bin_opcode::OPCODE_LAND:
        return llvm::ArithType::LAND;
    case bin_opcode::OPCODE_LOR:
        return llvm::ArithType::LOR;
    case bin_opcode::OPCODE_XOR:
        return llvm::ArithType::XOR;
    case bin_opcode::OPCODE_LSH:
        return llvm::ArithType::LSH;
    case bin_opcode::OPCODE_RSH:
        return llvm::ArithType::RSH;
    default:
        BYE_BYE(llvm::ArithType, "Called with non-arithmetic operation");
    }
}

static llvm::UnaryArithType convert(un_opcode op) {
    switch(op) {
    case un_opcode::OPCODE_BNOT:
        return llvm::UnaryArithType::BNOT;
    case un_opcode::OPCODE_NOT:
        return llvm::UnaryArithType::NOT;
    case un_opcode::OPCODE_NEG:
        return llvm::UnaryArithType::NEG;
    case un_opcode::OPCODE_LOAD:
        BYE_BYE(llvm::UnaryArithType, "Dereference operator not supported");
    }
}

class TermConstructor : public anno::empty_visitor {

    TermFactory* tf;
    Term::Ptr term;

public:

    TermConstructor(TermFactory* tf): tf(tf), term(nullptr) {}

    virtual void onDoubleConstant(double c) {
        term = tf->getOpaqueConstantTerm(c);
    }
    virtual void onIntConstant(int c) {
        term = tf->getOpaqueConstantTerm((long long)c);
    }
    virtual void onBoolConstant(bool v) {
        term = tf->getOpaqueConstantTerm(v);
    }
    virtual void onVariable(const std::string& name) {
        term = tf->getOpaqueVarTerm(name);
    }
    virtual void onBuiltin(const std::string& name) {
        term = tf->getOpaqueBuiltinTerm(name);
    }
    virtual void onMask(const std::string& mask) {
        term = tf->getOpaqueBuiltinTerm(mask);
    }

    virtual void onBinary(
            bin_opcode op,
            const prod_t& lhv,
            const prod_t& rhv) {
        TermConstructor lhvtc(tf);
        TermConstructor rhvtc(tf);

        lhv->accept(lhvtc);
        rhv->accept(rhvtc);

        if (isCompare(op)) {
            term = tf->getCmpTerm(convertCT(op), lhvtc.term, rhvtc.term );
        } else {
            term = tf->getBinaryTerm(convertAT(op), lhvtc.term, rhvtc.term );
        }
    }

    virtual void onUnary(
            un_opcode op,
            const prod_t& rhv) {
        TermConstructor rhvtc(tf);

        rhv->accept(rhvtc);
        if(op == un_opcode::OPCODE_LOAD) {
            term = tf->getLoadTerm(rhvtc.term);
        } else {
            term = tf->getUnaryTerm(convert(op), rhvtc.term);
        }
    }

    Term::Ptr getTerm() { return term; }
};

Annotation::Ptr borealis::fromParseResult(
        const Locus& locus, const anno::command& cmd, TermFactory* tf) {

    std::vector<Term::Ptr> terms;
    terms.reserve(cmd.args_.size());

    for (auto& arg : cmd.args_) {
        TermConstructor tc(tf);
        arg->accept(tc);
        terms.push_back(tc.getTerm());
    }

#define HANDLE_ANNOTATION(CMD, CLASS) \
    if (cmd.name_ == CMD) { \
        return CLASS::fromTerms(locus, terms); \
    }
#include "Annotation/Annotation.def"

    BYE_BYE(Annotation::Ptr, "Unknown annotation type: \"@" + cmd.name_ + "\"");
}

#include "Util/unmacros.h"
