/*
 * OpaqueBuiltinTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEBUILTINTERM_H_
#define OPAQUEBUILTINTERM_H_

#include "Term/Term.h"

#include "Util/macros.h"

namespace borealis {

class OpaqueBuiltinTerm: public borealis::Term {

    const std::string vname;

    OpaqueBuiltinTerm(const std::string& vname):
        Term(
            std::hash<std::string>()(vname),
            vname,
            type_id(*this)
        ), vname(vname) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueBuiltinTerm);

    const std::string& getName() const { return vname; }

    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    that->vname == vname;
        } else return false;
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().getUnknown();
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueBuiltinTerm> {
    static Dynamic<Impl> doit(
            const OpaqueBuiltinTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* OPAQUEBUILTINTERM_H_ */
