/*
 * Symbols.h
 *
 *  Created on: Aug 20, 2013
 *      Author: sam
 */

#ifndef MATHSAT_SYMBOLS_H_
#define MATHSAT_SYMBOLS_H_

#include <gmp.h>

#include "Factory/Nest.h"
#include "Logging/logger.hpp"
#include "Predicate/PredicateFactory.h"
#include "SMT/MathSAT/MathSAT.h"
#include "SMT/MathSAT/MathSatTypes.h"
#include "Term/Term.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {
namespace mathsat_ {
namespace unlogic {

class AbstractSymbol {
public:
    typedef std::shared_ptr<AbstractSymbol> Ptr;

protected:
    msat_symbol_tag symbolTag_;
    unsigned numArgs_;
    std::vector<AbstractSymbol::Ptr> args_;
    mathsat::Expr expr_;

public:
    AbstractSymbol(msat_symbol_tag symbolTag, unsigned numArgs, const mathsat::Expr& expr)
        : symbolTag_(symbolTag), numArgs_(numArgs), expr_(expr) {
        args_.resize(numArgs);
    }

    virtual ~AbstractSymbol() {}

    msat_symbol_tag symbolTag() const { return symbolTag_; };

    unsigned numArgs() const { return numArgs_; }

    bool isTerminal() const { return numArgs_ == 0; }

    const mathsat::Expr& expr() const { return expr_; }

    const std::vector<AbstractSymbol::Ptr>& args() const { return args_; }

    void setArg(unsigned num, AbstractSymbol::Ptr symbol) {
        ASSERTC(num < numArgs_);
        args_[num] = symbol;
    }

    virtual Term::Ptr undoThat(FactoryNest FN) const = 0;
};

////////////////////////////////////////////////////////////////////////////////

class ValueSymbol : public AbstractSymbol {
private:
    std::string name_;
public:
    ValueSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_UNKNOWN, 0, expr),
          name_(expr_.decl().name()) {}

    std::string name() const { return name_; }

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        if (expr_.is_bool()) {
            return FN.Term->getValueTerm(FN.Type->getBool(), name_);
        }
        // FIXME sam Should values always convert to integers?
        return FN.Term->getValueTerm(FN.Type->getInteger(), name_);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TrueSymbol : public AbstractSymbol {
public:
    TrueSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_TRUE, 0, expr) {}

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        return FN.Term->getBooleanTerm(true);
    }
};

////////////////////////////////////////////////////////////////////////////////

class FalseSymbol : public AbstractSymbol {
public:
    FalseSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_FALSE, 0, expr) {}

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        return FN.Term->getBooleanTerm(false);
    }
};

////////////////////////////////////////////////////////////////////////////////

class ConstantSymbol : public AbstractSymbol {
public:
    ConstantSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_FALSE, 0, expr) {}

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        USING_SMT_LOGIC(MathSAT);

        // FIXME sam This is VERY FUCKED UP!
        // Первый способ получить значение константы - это парсить
        // строковое представление ее имени:
        // - для бит-векторов это "a_b", где
        //     a - это значение бит-вектора,
        //     b - это размерность бит-вектора.
        // - для арифметических констант это "а", где
        //     а - это значение константы.
        //
        // if (expr_.is_bv()) {
        //     auto name = expr_.decl().name();
        //     auto value = name.substr(0, name.find("_"));
        //     auto size = expr_.get_sort().bv_size();
        //     if ( Integer::bitsize == size ) {
        //         return TFP->getIntTerm(std::stoi(value));
        //     } else if ( Real::bitsize == size ) {
        //         return TFP->getRealTerm(std::stod(value));
        //     }
        //     BYE_BYE(Term::Ptr, "Unknown value type");
        // }

        // FIXME sam Второй способ получения значения константы - при помощи msat_term_to_number()
        mpq_t q;
        mpq_init(q);
        msat_term_to_number(expr_.env(), expr_, q);
        if (mpz_get_si(mpq_denref(q)) == 1) {
            return FN.Term->getIntTerm(mpz_get_si(mpq_numref(q)));
        } else {
            return FN.Term->getRealTerm(mpq_get_d(q));
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class UnaryOperationSymbol : public AbstractSymbol {
public:
    UnaryOperationSymbol(msat_symbol_tag tag, const mathsat::Expr& expr)
        : AbstractSymbol(tag, 1, expr) {}

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        return FN.Term->getUnaryTerm(
            unaryType(),
            args_[0]->undoThat(FN)
        );
    }

private:
    llvm::UnaryArithType unaryType() const {
        using llvm::UnaryArithType;
        switch (symbolTag_) {
        case MSAT_TAG_NOT:    return UnaryArithType::NOT;
        case MSAT_TAG_BV_NOT: return UnaryArithType::BNOT;
        case MSAT_TAG_BV_NEG: return UnaryArithType::NEG;
        default: BYE_BYE(llvm::UnaryArithType, "Unsupported unary operation");
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class IffOperationSymbol : public AbstractSymbol {
public:
    IffOperationSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_IFF, 2, expr) {}

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        return FN.Term->getUnaryTerm(
            llvm::UnaryArithType::NOT,
            FN.Term->getBinaryTerm(
                llvm::ArithType::XOR,
                args_[0]->undoThat(FN),
                args_[1]->undoThat(FN)
            )
        );
    }
};

////////////////////////////////////////////////////////////////////////////////

class BinaryOperationSymbol : public AbstractSymbol {
public:
    BinaryOperationSymbol(msat_symbol_tag tag, const mathsat::Expr& expr)
        : AbstractSymbol(tag, 2, expr) {}

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        return FN.Term->getBinaryTerm(
            arithType(),
            args_[0]->undoThat(FN),
            args_[1]->undoThat(FN)
        );
    }

private:
    llvm::ArithType arithType() const {
        using llvm::ArithType;
        switch (symbolTag_) {
        case MSAT_TAG_AND:     return ArithType::LAND;
        case MSAT_TAG_OR:      return ArithType::LOR;
        case MSAT_TAG_TIMES:
        case MSAT_TAG_BV_MUL:  return ArithType::MUL;
        case MSAT_TAG_PLUS:
        case MSAT_TAG_BV_ADD:  return ArithType::ADD;
        case MSAT_TAG_BV_AND:  return ArithType::BAND;
        case MSAT_TAG_BV_OR:   return ArithType::BOR;
        case MSAT_TAG_BV_XOR:  return ArithType::XOR;
        case MSAT_TAG_BV_SUB:  return ArithType::SUB;
        case MSAT_TAG_BV_UDIV:
        case MSAT_TAG_BV_SDIV: return ArithType::DIV;
        case MSAT_TAG_BV_UREM:
        case MSAT_TAG_BV_SREM: return ArithType::REM;
        case MSAT_TAG_BV_LSHL: return ArithType::SHL;
        case MSAT_TAG_BV_LSHR: return ArithType::LSHR;
        case MSAT_TAG_BV_ASHR: return ArithType::ASHR;
        default: BYE_BYE(llvm::ArithType, "Unsupported binary operation");
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class CmpOperationSymbol : public AbstractSymbol {
public:
    CmpOperationSymbol(msat_symbol_tag tag, const mathsat::Expr& expr)
        : AbstractSymbol(tag, 2, expr) {}

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        return FN.Term->getCmpTerm(
            conditionType(),
            args_[0]->undoThat(FN),
            args_[1]->undoThat(FN)
        );
    }

private:
    llvm::ConditionType conditionType() const {
        using llvm::ConditionType;
        switch (symbolTag_) {
        case MSAT_TAG_EQ:     return ConditionType::EQ;
        case MSAT_TAG_LEQ:
        case MSAT_TAG_BV_SLE: return ConditionType::LE;
        case MSAT_TAG_BV_SLT: return ConditionType::LT;
        case MSAT_TAG_BV_ULE: return ConditionType::ULE;
        case MSAT_TAG_BV_ULT: return ConditionType::ULT;
        default: BYE_BYE(llvm::ConditionType, "Unsupported comparison operation");
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class IteOperationSymbol : public AbstractSymbol {
public:
    IteOperationSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_ITE, 3, expr) {}

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        return FN.Term->getTernaryTerm(
            args_[0]->undoThat(FN),
            args_[1]->undoThat(FN),
            args_[2]->undoThat(FN));
    }
};

////////////////////////////////////////////////////////////////////////////////

class LoadSymbol : public AbstractSymbol {

    static int idx_;

public:
    LoadSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_UNKNOWN, expr.num_args(), expr) {}

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        ASSERT("$$__initial_mem__$$" == expr_.decl().name() && 1 == numArgs_,
               "Only $$_initial_mem_$$ UF is supported");

        if (args_[0]->isTerminal()) {
            return FN.Term->getLoadTerm(
                FN.Term->getValueTerm(
                    FN.Type->getPointer(FN.Type->getInteger()),
                    args_[0]->undoThat(FN)->getName()
                )
            );
        } else {
            auto name = "$$__initial_mem_idx_" + util::toString(LoadSymbol::idx_++) + "__$$";
            auto idx = FN.Term->getValueTerm(
                FN.Type->getPointer(FN.Type->getInteger()),
                name
            );
            auto axs = FN.Term->getCmpTerm(
                llvm::ConditionType::EQ,
                idx,
                args_[0]->undoThat(FN)
            );
            return FN.Term->getAxiomTerm(
                FN.Term->getLoadTerm(
                    idx
                ),
                axs
            );
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class ExtractSymbol : public AbstractSymbol {
public:
    ExtractSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_BV_EXTRACT, 1, expr) {}

    virtual Term::Ptr undoThat(FactoryNest FN) const override {
        return FN.Term->getSignTerm(
            args_[0]->undoThat(FN)
        );
    }
};

////////////////////////////////////////////////////////////////////////////////

AbstractSymbol::Ptr SymbolFactory(const mathsat::Expr& expr);

////////////////////////////////////////////////////////////////////////////////

} // namespace unlogic
} // namespace mathsat_
} // namespace borealis

#include "Util/unmacros.h"

#endif /* MATHSAT_SYMBOLS_H_ */
