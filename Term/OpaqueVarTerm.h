/*
 * OpaqueVarTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEVARTERM_H_
#define OPAQUEVARTERM_H_

#include "Term/Term.h"

#include "Util/macros.h"

namespace borealis {

class OpaqueVarTerm: public borealis::Term {

    typedef OpaqueVarTerm Self;

    std::string vname;

    OpaqueVarTerm(const std::string& vname):
        Term(
             std::hash<std::string>()(vname),
             vname,
             type_id(*this)
        ), vname(vname) {};

public:

    const std::string& getName() const { return vname; }

    OpaqueVarTerm(const Self&) = default;
    virtual ~OpaqueVarTerm() {};

    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<Self>();
    }

    static bool classof(const Self*) {
        return true;
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    that->vname == vname;
        } else return false;
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().getUnknown();
    }

    friend class TermFactory;

};

template<class Impl>
struct SMTImpl<Impl, OpaqueVarTerm> {
    static Dynamic<Impl> doit(
            const OpaqueVarTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* OPAQUEVARTERM_H_ */
