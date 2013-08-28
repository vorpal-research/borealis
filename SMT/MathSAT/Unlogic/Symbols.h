/*
 * Symbols.h
 *
 *  Created on: Aug 20, 2013
 *      Author: sam
 */

#ifndef MATHSAT_SYMBOLS_H_
#define MATHSAT_SYMBOLS_H_

#include <gmp.h>

#include "Predicate/PredicateFactory.h"
#include "SMT/MathSAT/MathSAT.h"
#include "SMT/MathSAT/MathSatTypes.h"
#include "State/PredicateStateBuilder.h"
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

    void setArg(unsigned num, AbstractSymbol::Ptr symbol) {
        ASSERTC(num < numArgs_);
        if ( num + 1 > args_.size() ) {
            args_.push_back(symbol);
            return;
        }
        args_[num] = symbol;
    }

    virtual Term::Ptr undoThat(PredicateStateBuilder& stateBuilder) const = 0;
};

////////////////////////////////////////////////////////////////////////////////

class ValueSymbol : public AbstractSymbol {
private:
    std::string name_;
public:
    ValueSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_UNKNOWN, 0, expr), name_(expr_.decl().name()) {}

    std::string name() const { return name_; }

    virtual Term::Ptr undoThat(PredicateStateBuilder&) const override {
        auto TyFP = TypeFactory::get();
        auto TFP = TermFactory::get(nullptr, TyFP);

        if (expr_.is_bool()) {
            return TFP->getValueTerm(TyFP->getBool(), name_);
        }
        // FIXME sam Values always converts to Integers.
        return TFP->getValueTerm(TyFP->getInteger(), name_);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TrueSymbol : public AbstractSymbol {
public:
    TrueSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_TRUE, 0, expr) {}

    virtual Term::Ptr undoThat(PredicateStateBuilder&) const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getBooleanTerm(true);
    }
};

////////////////////////////////////////////////////////////////////////////////

class FalseSymbol : public AbstractSymbol {
public:
    FalseSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_FALSE, 0, expr) {}

    virtual Term::Ptr undoThat(PredicateStateBuilder&) const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getBooleanTerm(false);
    }
};

////////////////////////////////////////////////////////////////////////////////

class ConstantSymbol : public AbstractSymbol {
public:
    ConstantSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_FALSE, 0, expr) {}

    virtual Term::Ptr undoThat(PredicateStateBuilder&) const override {
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

class UnaryOperationSymbol : public AbstractSymbol {
public:
    UnaryOperationSymbol(msat_symbol_tag tag, const mathsat::Expr& expr)
        : AbstractSymbol(tag, 1, expr) {}

    virtual Term::Ptr undoThat(PredicateStateBuilder& stateBuilder) const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getUnaryTerm(unaryType(),
                                  args_[0]->undoThat(stateBuilder));
    }

private:
    llvm::UnaryArithType unaryType() const {
        using llvm::UnaryArithType;
        switch (symbolTag_) {
        case MSAT_TAG_NOT: return UnaryArithType::NOT;
        case MSAT_TAG_BV_NOT: return UnaryArithType::BNOT;
        case MSAT_TAG_BV_NEG: return UnaryArithType::NEG;
        default: BYE_BYE(llvm::UnaryArithType, "Unsupported unary operation.");
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class IffOperationSymbol : public AbstractSymbol {
public:
    IffOperationSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_IFF, 2, expr) {}

    virtual Term::Ptr undoThat(PredicateStateBuilder& stateBuilder) const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        auto xor_ = TFP->getBinaryTerm(llvm::ArithType::XOR,
                                  args_[0]->undoThat(stateBuilder),
                                  args_[1]->undoThat(stateBuilder));
        return TFP->getUnaryTerm(llvm::UnaryArithType::NOT, xor_);
    }
};

////////////////////////////////////////////////////////////////////////////////

class BinaryOperationSymbol : public AbstractSymbol {
public:
    BinaryOperationSymbol(msat_symbol_tag tag, const mathsat::Expr& expr)
        : AbstractSymbol(tag, 2, expr) {}

    virtual Term::Ptr undoThat(PredicateStateBuilder& stateBuilder) const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getBinaryTerm(arithType(),
                                  args_[0]->undoThat(stateBuilder),
                                  args_[1]->undoThat(stateBuilder));
    }

private:
    llvm::ArithType arithType() const {
        using llvm::ArithType;
        switch (symbolTag_) {
        case MSAT_TAG_AND: return ArithType::LAND;
        case MSAT_TAG_OR: return ArithType::LOR;
        case MSAT_TAG_TIMES: case MSAT_TAG_BV_MUL: return ArithType::MUL;
        case MSAT_TAG_PLUS: case MSAT_TAG_BV_ADD: return ArithType::ADD;
        case MSAT_TAG_BV_AND: return ArithType::BAND;
        case MSAT_TAG_BV_OR: return ArithType::BOR;
        case MSAT_TAG_BV_XOR: return ArithType::XOR;
        case MSAT_TAG_BV_SUB: return ArithType::SUB;
        case MSAT_TAG_BV_UDIV: case MSAT_TAG_BV_SDIV: return ArithType::DIV;
        case MSAT_TAG_BV_UREM: case MSAT_TAG_BV_SREM: return ArithType::REM;
        case MSAT_TAG_BV_LSHL: return ArithType::SHL;
        case MSAT_TAG_BV_LSHR: return ArithType::LSHR;
        case MSAT_TAG_BV_ASHR: return ArithType::ASHR;
        default: BYE_BYE(llvm::ArithType, "Unsupported binary operation.");
        }
    }
};


////////////////////////////////////////////////////////////////////////////////

class CmpOperationSymbol : public AbstractSymbol {
public:
    CmpOperationSymbol(msat_symbol_tag tag, const mathsat::Expr& expr)
        : AbstractSymbol(tag, 2, expr) {}

    virtual Term::Ptr undoThat(PredicateStateBuilder& stateBuilder) const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getCmpTerm(conditionType(),
                                  args_[0]->undoThat(stateBuilder),
                                  args_[1]->undoThat(stateBuilder));
    }

private:
    llvm::ConditionType conditionType() const {
        using llvm::ConditionType;
        switch (symbolTag_) {
        case MSAT_TAG_EQ: return ConditionType::EQ;
        case MSAT_TAG_LEQ: case MSAT_TAG_BV_SLE:  return ConditionType::LE;
        case MSAT_TAG_BV_SLT:  return ConditionType::LT;
        case MSAT_TAG_BV_ULE: return ConditionType::ULE;
        case MSAT_TAG_BV_ULT:  return ConditionType::ULT;
        default: BYE_BYE(llvm::ConditionType, "Unsupported comparison operation.");
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class IteOperationSymbol : public AbstractSymbol {
public:
    IteOperationSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_ITE, 3, expr) {}

    virtual Term::Ptr undoThat(PredicateStateBuilder& stateBuilder) const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getTernaryTerm(args_[0]->undoThat(stateBuilder),
                                   args_[1]->undoThat(stateBuilder),
                                   args_[2]->undoThat(stateBuilder));
    }
};

////////////////////////////////////////////////////////////////////////////////

class LoadSymbol : public AbstractSymbol {

    static int idx_;

public:
    LoadSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_UNKNOWN, expr.num_args(), expr) {}

    virtual Term::Ptr undoThat(PredicateStateBuilder& stateBuilder) const override {
        ASSERT(expr_.decl().name() == "$$__initial_mem__$$",
                "Can convert only $$_initial_mem_$$ function to load term.");
        ASSERT(numArgs_ == 1,
                "Can convert only $$_initial_mem_$$ function to load term.");

        auto TyFP = TypeFactory::get();
        auto TFP = TermFactory::get(nullptr, TyFP);
        auto PF = PredicateFactory::get();

        if (args_[0]->isTerminal()) {
            auto argTerm = args_[0]->undoThat(stateBuilder);
            auto ptr = TFP->getValueTerm(TyFP->getPointer(TyFP->getInteger()), argTerm->getName());
            return TFP->getLoadTerm(ptr);
        } else {
            auto name = "$$__initial_mem_idx_" + util::toString(LoadSymbol::idx_) + "__$$";
            LoadSymbol::idx_++;
            auto idx = TFP->getValueTerm(TyFP->getPointer(TyFP->getInteger()), name);
            auto equal = TFP->getCmpTerm(llvm::ConditionType::EQ, idx, args_[0]->undoThat(stateBuilder));
            auto axiom = TFP->getAxiomTerm(TFP->getTrueTerm(), equal);
            stateBuilder += PF->getEqualityPredicate(axiom, TFP->getBooleanTerm(true));
            return TFP->getLoadTerm(idx);
        }
    }
};

int LoadSymbol::idx_ = 0;

////////////////////////////////////////////////////////////////////////////////

class ExtractSymbol : public AbstractSymbol {
public:
    ExtractSymbol(const mathsat::Expr& expr)
        : AbstractSymbol(MSAT_TAG_BV_EXTRACT, 1, expr) {}

    virtual Term::Ptr undoThat(PredicateStateBuilder& stateBuilder) const override {
        auto TFP = TermFactory::get(nullptr, TypeFactory::get());
        return TFP->getSignTerm(args_[0]->undoThat(stateBuilder));
    }
};

////////////////////////////////////////////////////////////////////////////////

AbstractSymbol::Ptr SymbolFactory(const mathsat::Expr& expr) {
    if (expr.num_args() == 0) {
        if (msat_term_is_true(expr.env(), expr)) {
            return AbstractSymbol::Ptr{ new TrueSymbol(expr) };
        } else if (msat_term_is_false(expr.env(), expr)) {
            return AbstractSymbol::Ptr{ new FalseSymbol(expr) };
        } else if (msat_term_is_uf(expr.env(), expr)) {
            BYE_BYE(AbstractSymbol::Ptr, "Can't revert uinterpreted function.");
        } else if (msat_term_is_number(expr.env(), expr)) {
            return AbstractSymbol::Ptr{ new ConstantSymbol(expr) };
        } else if (msat_term_is_constant(expr.env(), expr)) {
            return AbstractSymbol::Ptr{ new ValueSymbol(expr) };
        } else {
            BYE_BYE(AbstractSymbol::Ptr, "Unknown expr type with 0 arguments.");
        }
    }

    auto tag = expr.decl().tag();
    switch (tag) {
    case MSAT_TAG_ITE:
        return AbstractSymbol::Ptr{ new IteOperationSymbol(expr) };

    case MSAT_TAG_IFF:
        return AbstractSymbol::Ptr{ new IffOperationSymbol(expr) };

    case MSAT_TAG_NOT: case MSAT_TAG_BV_NOT: case MSAT_TAG_BV_NEG:
        return AbstractSymbol::Ptr{ new UnaryOperationSymbol(tag, expr) };

    case MSAT_TAG_AND: case MSAT_TAG_OR: case MSAT_TAG_TIMES: case MSAT_TAG_BV_MUL:
    case MSAT_TAG_PLUS: case MSAT_TAG_BV_ADD: case MSAT_TAG_BV_AND: case MSAT_TAG_BV_OR:
    case MSAT_TAG_BV_XOR: case MSAT_TAG_BV_SUB: case MSAT_TAG_BV_UDIV: case MSAT_TAG_BV_SDIV:
    case MSAT_TAG_BV_UREM: case MSAT_TAG_BV_SREM: case MSAT_TAG_BV_LSHL: case MSAT_TAG_BV_LSHR:
    case MSAT_TAG_BV_ASHR:
        return AbstractSymbol::Ptr{ new BinaryOperationSymbol(tag, expr) };

    case MSAT_TAG_EQ: case MSAT_TAG_LEQ: case MSAT_TAG_BV_SLE:
    case MSAT_TAG_BV_SLT: case MSAT_TAG_BV_ULE: case MSAT_TAG_BV_ULT:
        return AbstractSymbol::Ptr{ new CmpOperationSymbol(tag, expr) };

    case MSAT_TAG_BV_EXTRACT:
        return AbstractSymbol::Ptr{ new ExtractSymbol(expr) };

    default:
        if (msat_term_is_uf(expr.env(), expr)) {
            return AbstractSymbol::Ptr{ new LoadSymbol(expr) };
        }
        dbgs() << "Unknown expr: " << expr << endl;
        BYE_BYE(AbstractSymbol::Ptr, "Unknown expr type.");
    }
}


} // namespace unlogic
} // namespace mathsat_
} // namespace borealis

#include "Util/unmacros.h"

#endif /* MATHSAT_SYMBOLS_H_ */
