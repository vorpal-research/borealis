/*
 * AllocaPredicate.h
 *
 *  Created on: Nov 28, 2012
 *      Author: belyaev
 */

#ifndef ALLOCAPREDICATE_H_
#define ALLOCAPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class AllocaPredicate: public borealis::Predicate {

    typedef AllocaPredicate Self;

public:

    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getNumElems() const { return numElements; }

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<Self>();
    }

    static bool classof(const Self*) {
        return true;
    }

    template<class SubClass>
    const Self* accept(Transformer<SubClass>* t) const {
        return new Self{
            t->transform(lhv),
            t->transform(numElements),
            this->type
        };
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

    virtual Predicate* clone() const override {
        return new Self{ *this };
    }

    friend class PredicateFactory;

private:

    Term::Ptr lhv;
    Term::Ptr numElements;

    AllocaPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            PredicateType type = PredicateType::STATE);
    AllocaPredicate(const Self&) = default;

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, AllocaPredicate> {
    static Bool<Impl> doit(
            const AllocaPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        auto lhve = SMT<Impl>::doit(p->getLhv(), ef, ctx).template to<Pointer>();

        ASSERT(!lhve.empty(), "Encountered alloca with non-Pointer left side");

        unsigned long long elems = 1;
        if (const ConstTerm* cnst = llvm::dyn_cast<ConstTerm>(p->getNumElems())) {
            if (llvm::ConstantInt* intCnst = llvm::dyn_cast<llvm::ConstantInt>(cnst->getConstant())) {
                elems = intCnst->getLimitedValue();
            } else ASSERT(false, "Encountered alloca with non-integer element number");
        } else if (const OpaqueIntConstantTerm* cnst = llvm::dyn_cast<OpaqueIntConstantTerm>(p->getNumElems())) {
            elems = cnst->getValue();
        } else ASSERT(false, "Encountered alloca with non-integer/non-constant element number");

        auto lhvp = lhve.getUnsafe();
        return lhvp == ctx->getDistinctPtr(elems);
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* ALLOCAPREDICATE_H_ */
