/*
 * TermFactory.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef TERMFACTORY_H_
#define TERMFACTORY_H_

#include <memory>

#include "Codegen/llvm.h"
#include "Term/Term.def"
#include "Type/TypeFactory.h"
#include "Util/slottracker.h"

namespace borealis {

class TermFactory {

    using ValueVector = std::vector<llvm::Value*>;

public:

    using Ptr = std::shared_ptr<TermFactory>;

    Term::Ptr getArgumentTerm(const llvm::Argument* arg, llvm::Signedness sign = llvm::Signedness::Unknown);
    Term::Ptr getStringArgumentTerm(const llvm::Argument* arg, llvm::Signedness sign = llvm::Signedness::Unknown);
    Term::Ptr getArgumentTermExternal(size_t numero, const std::string& name, Type::Ptr type);
    Term::Ptr getArgumentCountTerm();
    Term::Ptr getVarArgumentTerm(size_t numero);

    Term::Ptr getConstTerm(const llvm::Constant* c, llvm::Signedness sign = llvm::Signedness::Unknown);

    Term::Ptr getNullPtrTerm();
    Term::Ptr getNullPtrTerm(const llvm::ConstantPointerNull* n);
    Term::Ptr getUndefTerm(const llvm::UndefValue* u);
    Term::Ptr getInvalidPtrTerm();

    Term::Ptr getBooleanTerm(bool b);
    Term::Ptr getTrueTerm();
    Term::Ptr getFalseTerm();

    Term::Ptr getIntTerm(int64_t value, unsigned int size, llvm::Signedness sign = llvm::Signedness::Unknown);
    Term::Ptr getIntTerm(const std::string& value, unsigned int size, llvm::Signedness sign = llvm::Signedness::Unknown);
    Term::Ptr getIntTerm(const llvm::APInt& value, llvm::Signedness sign = llvm::Signedness::Unknown);
    Term::Ptr getIntTerm(int64_t value, Type::Ptr type);

    Term::Ptr getRealTerm(double d);

    Term::Ptr getReturnValueTerm(const llvm::Function* F, llvm::Signedness sign = llvm::Signedness::Unknown);
    Term::Ptr getReturnPtrTerm(const llvm::Function* F);

    Term::Ptr getValueTerm(const llvm::Value* v, llvm::Signedness sign = llvm::Signedness::Unknown);
    Term::Ptr getValueTerm(Type::Ptr type, const std::string& name);
    Term::Ptr getGlobalValueTerm(const llvm::GlobalValue* gv, llvm::Signedness sign = llvm::Signedness::Unknown);
    Term::Ptr getLocalValueTerm(const llvm::Value* v, llvm::Signedness sign = llvm::Signedness::Unknown);

    Term::Ptr getTernaryTerm(Term::Ptr cnd, Term::Ptr tru, Term::Ptr fls);

    Term::Ptr getBinaryTerm(llvm::ArithType opc, Term::Ptr lhv, Term::Ptr rhv);

    Term::Ptr getUnaryTerm(llvm::UnaryArithType opc, Term::Ptr rhv);

    Term::Ptr getUnlogicLoadTerm(Term::Ptr rhv);
    Term::Ptr getLoadTerm(Term::Ptr rhv);
    Term::Ptr getReadPropertyTerm(Type::Ptr type, Term::Ptr propName, Term::Ptr rhv);

    Term::Ptr getGepTerm(Term::Ptr base, const std::vector<Term::Ptr>& shifts, bool isTriviallyInbounds = false);
    Term::Ptr getGepTerm(llvm::Value* base, const ValueVector& idxs, bool isTriviallyInbounds = false);

    Term::Ptr getCmpTerm(llvm::ConditionType opc, Term::Ptr lhv, Term::Ptr rhv);

    Term::Ptr getOpaqueVarTerm(const std::string& name);
    Term::Ptr getOpaqueBuiltinTerm(const std::string& name);
    Term::Ptr getOpaqueNamedConstantTerm(const std::string& name);
    Term::Ptr getOpaqueConstantTerm(int64_t v, size_t bitSize = 0x0);
    Term::Ptr getOpaqueConstantTerm(double v);
    Term::Ptr getOpaqueConstantTerm(bool v);
    Term::Ptr getOpaqueConstantTerm(const char* v);
    Term::Ptr getOpaqueConstantTerm(const std::string& v);
    Term::Ptr getOpaqueIndexingTerm(Term::Ptr lhv, Term::Ptr rhv);
    Term::Ptr getOpaqueMemberAccessTerm(Term::Ptr lhv, const std::string& property, bool indirect = false);
    Term::Ptr getOpaqueCallTerm(Term::Ptr lhv, const std::vector<Term::Ptr>& rhv);

    Term::Ptr getSignTerm(Term::Ptr rhv);

    Term::Ptr getCastTerm(Type::Ptr type, bool signExtend, Term::Ptr rhv);

    Term::Ptr getAxiomTerm(Term::Ptr lhv, Term::Ptr rhv);

    Term::Ptr getBoundTerm(Term::Ptr rhv);

    static TermFactory::Ptr get(SlotTracker* st, const llvm::DataLayout* DL, TypeFactory::Ptr TyF);
    static TermFactory::Ptr get(const llvm::DataLayout* DL, TypeFactory::Ptr TyF);

private:

    SlotTracker* st;
    const llvm::DataLayout* DL;
    TypeFactory::Ptr TyF;


    TermFactory(SlotTracker* st, const llvm::DataLayout* DL, TypeFactory::Ptr TyF);

};

} /* namespace borealis */

#endif /* TERMFACTORY_H_ */
