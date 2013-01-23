/*
 * OpaqueVarTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEVARTERM_H_
#define OPAQUEVARTERM_H_

#include "Term.h"
#include "NameContext.h"

namespace borealis {

class OpaqueVarTerm: public borealis::Term {
    typedef OpaqueVarTerm self;

    std::string vname;

    OpaqueVarTerm(const std::string& vname):
        Term(
             std::hash<std::string>()(vname),
             llvm::ValueType::UNKNOWN,
             vname,
             type_id(*this)
        ),
        vname(vname){};


public:
    OpaqueVarTerm(const OpaqueVarTerm&) = default;

    const std::string& getName() const { return vname; }

    friend class TermFactory;

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
        if(const self* that = llvm::dyn_cast<self>(other)) {
            return  Term::equals(other) &&
                    that->vname == vname;
        } else return false;
    }
};

} /* namespace borealis */
#endif /* OPAQUEVARTERM_H_ */
