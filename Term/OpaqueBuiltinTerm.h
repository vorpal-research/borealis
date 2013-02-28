/*
 * OpaqueBuiltinTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEBUILTINTERM_H_
#define OPAQUEBUILTINTERM_H_

#include "Term/Term.h"

namespace borealis {

class OpaqueBuiltinTerm: public borealis::Term {

    typedef OpaqueBuiltinTerm self;

    const std::string vname;

    OpaqueBuiltinTerm(const std::string& vname):
        Term(
            std::hash<std::string>()(vname),
            llvm::ValueType::UNKNOWN,
            vname,
            type_id(*this)
        ), vname(vname) {};

public:

    const std::string& getName() const { return vname; }

    OpaqueBuiltinTerm(const OpaqueBuiltinTerm&) = default;

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }

    virtual bool equals(const Term* other) const {
        if (const self* that = llvm::dyn_cast<self>(other)) {
            return  Term::equals(other) &&
                    that->vname == vname;
        } else return false;
    }

    virtual Type::Ptr getTermType() const {
        return TypeFactory::getInstance().getUnknown();
    }

    friend class TermFactory;

};

} /* namespace borealis */

#endif /* OPAQUEBUILTINTERM_H_ */
