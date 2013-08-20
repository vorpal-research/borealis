/*
 * Symbols.h
 *
 *  Created on: Aug 20, 2013
 *      Author: sam
 */

#ifndef MATHSAT_SYMBOLS_H_
#define MATHSAT_SYMBOLS_H_

#include <gmp.h>

#include "SMT/MathSAT/MathSAT.h"
#include "SMT/MathSAT/MathSatTypes.h"
#include "Term/Term.h"
#include "Term/TermFactory.h"
#include "Util/macros.h"
#include "Util/util.h"

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
                    : symbolTag_(symbolTag), numArgs_(numArgs),
                      args_(std::vector<AbstractSymbol::Ptr>()), expr_(expr) {
        args_.reserve(numArgs);
    }

    virtual ~AbstractSymbol() {}

    msat_symbol_tag symbolTag() const { return symbolTag_; };

    unsigned numArgs() const { return numArgs_; }

    bool isTerminal() const { return numArgs_ == 0; }

    const mathsat::Expr& expr() const { return expr_; }

    const std::vector<AbstractSymbol::Ptr>& args() const { return args_; }

//    void setArg(unsigned num, AbstractSymbol::Ptr symbol) { args_[num] = symbol; }
    void setArg(unsigned num, AbstractSymbol::Ptr symbol) {
        ASSERTC(num < numArgs_);
        if ( num + 1 > args_.size() ) {
            args_.push_back(symbol);
            return;
        }
        args_[num] = symbol;
    }

    virtual Term::Ptr undoThat() const = 0;
};

////////////////////////////////////////////////////////////////////////////////

class ValueSymbol : public AbstractSymbol {
public:
    ValueSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_UNKNOWN, 0, expr) {}

    virtual Term::Ptr undoThat() const override {
        USING_SMT_LOGIC(MathSAT);
        auto TyFP = TypeFactory::get();
        auto TFP = TermFactory::get(nullptr, TyFP);

        auto name = expr_.decl().name();
        if (expr_.is_bool()) {
            return TFP->getValueTerm(TyFP->getBool(), name);
        }

        auto size = expr_.get_sort().bv_size();
        if ( Pointer::bitsize == size ) {
            //Pointer to Integer by default.
            return TFP->getValueTerm(TyFP->getPointer(TyFP->getInteger()), name);
        }else if ( Integer::bitsize == size ) {
            return TFP->getValueTerm(TyFP->getInteger(), name);
        }else if ( Real::bitsize == size ) {
            return TFP->getValueTerm(TyFP->getFloat(), name);
        }
        BYE_BYE(Term::Ptr, "Unknown value type");
    }
};

////////////////////////////////////////////////////////////////////////////////

class TrueSymbol : public AbstractSymbol {
public:
    TrueSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_TRUE, 0, expr) {}

    virtual Term::Ptr undoThat() const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getBooleanTerm(true);
    }
};

////////////////////////////////////////////////////////////////////////////////

class FalseSymbol : public AbstractSymbol {
public:
    FalseSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_FALSE, 0, expr) {}

    virtual Term::Ptr undoThat() const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getBooleanTerm(false);
    }
};

////////////////////////////////////////////////////////////////////////////////

class ConstantSymbol : public AbstractSymbol {
public:
    ConstantSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_FALSE, 0, expr) {}

    virtual Term::Ptr undoThat() const override {
        USING_SMT_LOGIC(MathSAT);
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());

        // FIXME sam This is VERY Fucked up!!! Первый способ получить значение
        // константы это парсить строковое представление ее имени.
        // Для бит-векторов это "a_b", где a -это значение, b - это размерность
        // бит-вектора. Для арифметических констант имя - это просто "а",
        // где а - это значение константы.
        // if (expr_.is_bv()) {
        //     auto name = expr_.decl().name();
        //     auto value = name.substr(0, name.find("_"));
        //     auto size = expr_.get_sort().bv_size();
        //     if ( Integer::bitsize == size ) {
        //         return TFP->getIntTerm(std::stoi(value));
        //     } else if ( Real::bitsize == size ) {
        //         return TFP->getIntTerm(std::stoi(value));
        //     }
        //     BYE_BYE(Term::Ptr, "Unknown value type");
        // }
        // return TFP->getIntTerm(std::stoi(expr_.decl().name()));

        // FIXME sam Второй способ получения значения константы - при помощи
        // msat_term_to_number().
        mpq_t q;
        mpq_init(q);
        msat_term_to_number(expr_.env(), expr_, q);
        if (mpz_get_si(mpq_denref(q)) == 1) {
            return TFP->getIntTerm(mpz_get_si(mpq_numref(q)));
        } else {
            return TFP->getRealTerm(mpq_get_d(q));
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class EqualitySymbol : public AbstractSymbol {
public:
    EqualitySymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_EQ, 2, expr) {}

    virtual Term::Ptr undoThat() const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getCmpTerm(llvm::ConditionType::EQ,
                                  args_[0]->undoThat(),
                                  args_[1]->undoThat());
    }
};

////////////////////////////////////////////////////////////////////////////////

class BvPlusSymbol : public AbstractSymbol {
public:
    BvPlusSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_BV_ADD, 2, expr) {}

    virtual Term::Ptr undoThat() const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getBinaryTerm(llvm::ArithType::ADD,
                                  args_[0]->undoThat(),
                                  args_[1]->undoThat());
    }
};

////////////////////////////////////////////////////////////////////////////////

AbstractSymbol::Ptr SymbolFactory(const mathsat::Expr& expr) {
    if (expr.num_args() == 0) {
        switch (expr.decl().tag()) {
        case MSAT_TAG_TRUE: return AbstractSymbol::Ptr{ new TrueSymbol(expr) };
        case MSAT_TAG_FALSE: return AbstractSymbol::Ptr{ new FalseSymbol(expr) };
        case MSAT_TAG_UNKNOWN:
            if (msat_term_is_uf(expr.env(), expr)) {
                BYE_BYE(AbstractSymbol::Ptr, "Can't revert uinterpreted function.");
            } else if (msat_term_is_number(expr.env(), expr)) {
                return AbstractSymbol::Ptr{ new ConstantSymbol(expr) };
            } else if (msat_term_is_constant(expr.env(), expr)) {
                return AbstractSymbol::Ptr{ new ValueSymbol(expr) };
            } else {
                BYE_BYE(AbstractSymbol::Ptr, "Unknown expr type with 0 arguments.");
            }
        default: BYE_BYE(AbstractSymbol::Ptr, "Unknown expr type with 0 arguments.");
        }
    }

    switch (expr.decl().tag()) {
    case MSAT_TAG_BV_ADD: return AbstractSymbol::Ptr{ new BvPlusSymbol(expr) };
    case MSAT_TAG_EQ: return AbstractSymbol::Ptr{ new EqualitySymbol(expr) };
    default: BYE_BYE(AbstractSymbol::Ptr, "Unknown expr type.");
    }
}


} // namespace unlogic
} // namespace mathsat_
} // namespace borealis

#include "Util/unmacros.h"

#endif /* MATHSAT_SYMBOLS_H_ */
