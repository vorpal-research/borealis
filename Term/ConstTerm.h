/*
 * ConstTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef CONSTTERM_H_
#define CONSTTERM_H_

#include "Term.h"

namespace borealis {

class ConstTerm: public borealis::Term {

public:

    ConstTerm(llvm::ValueType type, const std::string& name) :
        Term(0, type, name, type_id(*this))
    {}
    virtual ~ConstTerm() {};

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<ConstTerm>();
    }

    static bool classof(const ConstTerm* /* t */) {
        return true;
    }

};

} /* namespace borealis */

#endif /* CONSTTERM_H_ */
