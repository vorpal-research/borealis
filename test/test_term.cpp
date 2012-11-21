/*
 * test_term.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>

#include "Term/ArgumentTerm.h"
#include "Term/ConstTerm.h"
#include "Term/ReturnValueTerm.h"
#include "Term/Term.h"
#include "Term/TermFactory.h"
#include "Term/ValueTerm.h"
#include "Util/slottracker.h"
#include "Util/util.h"

namespace {

using namespace borealis::util;
using namespace borealis::util::streams;

TEST(Term, classof) {

    {
        using namespace borealis;

        using llvm::LLVMContext;
        using llvm::Argument;
        using llvm::ArrayRef;
        using llvm::Function;
        using llvm::FunctionType;
        using llvm::GlobalValue;
        using llvm::Module;
        using llvm::Type;
        using llvm::Value;
        using llvm::dyn_cast;
        using llvm::isa;

        LLVMContext& ctx = llvm::getGlobalContext();
        Module m("mock-module", ctx);

        Function* f = Function::Create(
                FunctionType::get(
                        Type::getVoidTy(ctx),
                        ArrayRef<Type*>(Type::getInt1Ty(ctx)),
                        false),
                GlobalValue::LinkageTypes::ExternalLinkage);
        Argument* a = &*f->getArgumentList().begin();
        a->setName("mock-arg");

        SlotTracker st(&m);

        auto TF = TermFactory::get(&st);

        Term::Ptr t1 = TF->getArgumentTerm(a);

        EXPECT_TRUE(isa<ArgumentTerm>(*t1));
        EXPECT_TRUE(isa<Term>(*t1));
        EXPECT_FALSE(isa<ConstTerm>(*t1));
        EXPECT_FALSE(isa<ReturnValueTerm>(*t1));
        EXPECT_FALSE(isa<ValueTerm>(*t1));

        const ConstTerm* t2 = dyn_cast<ConstTerm>(t1.get());
        EXPECT_EQ(nullptr, t2);
    }

}

} // namespace borealis
