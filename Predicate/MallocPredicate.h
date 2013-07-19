/*
 * MallocPredicate.h
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#ifndef MALLOCPREDICATE_H_
#define MALLOCPREDICATE_H_

#include "Config/config.h"
#include "Predicate/Predicate.h"

namespace borealis {

class MallocPredicate: public borealis::Predicate {

    typedef MallocPredicate Self;

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

    MallocPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            PredicateType type = PredicateType::STATE);
    MallocPredicate(const Self&) = default;

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, MallocPredicate> {
    static Bool<Impl> doit(
            const MallocPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        auto lhve = SMT<Impl>::doit(p->getLhv(), ef, ctx).template to<Pointer>();
        ASSERT(!lhve.empty(), "Malloc produces a non-pointer");

        unsigned long long elems = 1;
        if (const ConstTerm* cnst = llvm::dyn_cast<ConstTerm>(p->getNumElems())) {
            if (llvm::ConstantInt* intCnst = llvm::dyn_cast<llvm::ConstantInt>(cnst->getConstant())) {
                elems = intCnst->getLimitedValue();
            } else ASSERT(false, "Encountered malloc with non-integer element number");
        } else if (const OpaqueIntConstantTerm* cnst = llvm::dyn_cast<OpaqueIntConstantTerm>(p->getNumElems())) {
            elems = cnst->getValue();
        } else ASSERT(false, "Encountered malloc with non-integer/non-constant element number");

        auto lhvp = lhve.getUnsafe();

        static config::ConfigEntry<bool> NullableMallocs("analysis", "nullable-mallocs");
        if(NullableMallocs.get(true)) {
            return lhvp == ef.getNullPtr() || lhvp == ctx->getDistinctPtr(elems);
        } else {
            return lhvp == ctx->getDistinctPtr(elems);
        }
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* MALLOCPREDICATE_H_ */
